// main.cpp
#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// ---------- ChunkVector -----------
template<typename T,std::size_t CHUNK>
class ChunkVector{
    struct Chunk{T data[CHUNK];};
    std::vector<Chunk*> blocks;
    std::size_t sz=0;
public:
    ~ChunkVector(){for(auto*p:blocks) std::free(p);}
    void push_back(const T&v){
        if(sz%CHUNK==0) blocks.push_back(static_cast<Chunk*>(std::aligned_alloc(64,sizeof(Chunk))));
        blocks[sz/CHUNK]->data[sz%CHUNK]=v;
        ++sz;
    }
    T&operator[](std::size_t i){return blocks[i/CHUNK]->data[i%CHUNK];}
    std::size_t size()const{return sz;}
};

// ---------- SecClock ---------------
struct SecStamp{uint64_t seq;uint64_t ns;};
class SecClock{
    std::atomic<uint64_t> counter{0};
public:
    SecStamp now(){
        uint64_t s=counter.fetch_add(1,std::memory_order_relaxed);
        uint64_t ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        return {s,ns};
    }
};

// ---------- SPSC queue in shared memory ------------
template<typename T,std::size_t CAP>
class SpscShmQueue{
    struct alignas(64) Cell{std::atomic<size_t> seq;T data;};
    struct Header{std::atomic<size_t> head;std::atomic<size_t> tail;Cell cells[CAP];};
    Header* h;
public:
    SpscShmQueue(const char*path,bool create){
        int fd=shm_open(path,O_RDWR|(create?O_CREAT:0),0666);
        ftruncate(fd,sizeof(Header));
        h=reinterpret_cast<Header*>(mmap(nullptr,sizeof(Header),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0));
        if(create){h->head.store(0);h->tail.store(0);for(size_t i=0;i<CAP;i++)h->cells[i].seq.store(i);}
        close(fd);
    }
    bool push(const T&v){
        size_t tail=h->tail.load(std::memory_order_relaxed);
        Cell&c=h->cells[tail%CAP];
        size_t seq=c.seq.load(std::memory_order_acquire);
        if(seq!=tail) return false;
        c.data=v;
        c.seq.store(tail+1,std::memory_order_release);
        h->tail.store(tail+1,std::memory_order_relaxed);
        return true;
    }
    bool pop(T&v){
        size_t head=h->head.load(std::memory_order_relaxed);
        Cell&c=h->cells[head%CAP];
        size_t seq=c.seq.load(std::memory_order_acquire);
        if(seq!=head+1) return false;
        v=c.data;
        c.seq.store(head+CAP,std::memory_order_release);
        h->head.store(head+1,std::memory_order_relaxed);
        return true;
    }
};

// ---------- Demo: producer + consumer in one process ------------
int main(){
    constexpr std::size_t QSIZE=1024;
    SpscShmQueue<SecStamp,QSIZE> q("/tradeq",true);
    SecClock clk;
    ChunkVector<SecStamp,64> log;
    for(size_t i=0;i<1'000'000;i++){
        SecStamp s=clk.now();
        while(!q.push(s));
    }
    SecStamp x;
    while(q.pop(x)) log.push_back(x);
    std::cout<<\"captured \"<<log.size()<<\" stamps\\n\";
}
