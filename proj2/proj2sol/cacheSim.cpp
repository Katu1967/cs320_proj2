#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <unordered_map>

using namespace std;

struct Trace_e{
    char type;
    unsigned long address;
};

struct Cache_line{
    bool valid = false;
    unsigned long tag = 0;
    unsigned long age = 0;
};

class DirectMap{
    protected:
        string name;
        unsigned long cache_size;
        unsigned long hits;
        unsigned long total_access;
        vector <Cache_line> table;
        int offset_bits;
        int index_mask;
        int block_size = 32; // 32 bytes per line
        int index_bits;
        
    public:
        double hit_rate;

        DirectMap(){
            this->cache_size = 0;
            this->hits = 0;
            this->total_access = 0;
            this->offset_bits = 5; 

        }

        DirectMap(unsigned long cache_size){

            if(cache_size == 1024){
                name = "direct map  1KB";
            }else if(cache_size == 4096){
                name = "direct map  4KB";
            }else if(cache_size == 16384){
                name = "direct map 16KB";
            }else if(cache_size == 32768){
                name = "direct map 32KB";
            }

            this->cache_size = cache_size;
            this->hits = 0;
            this->total_access = 0;
            this->table.resize(cache_size / 32);
            this->offset_bits = 5; 
            this->index_bits = log2(cache_size / 32);
            this->index_mask = (cache_size / 32) - 1;
        }

        virtual unsigned long get_index(unsigned long  address){
            return ((address >> offset_bits) & index_mask);
        }

        virtual unsigned long get_tag(unsigned long  address){
            return address >> (offset_bits + index_bits);
        }

        //gets the address and calculates the index and tag
        virtual void access(char type, unsigned long address){
                //calc index and tag
                unsigned long index = get_index(address);
                unsigned long tag = get_tag(address);


                Cache_line t = table[index];

                if(t.valid && t.tag == tag){

                    hits++;
                }else{
                    table[index].valid = true;
                    table[index].tag = tag;
                    
                }
                total_access++;
            

            return;
        }

        virtual void run_sim(const vector<Trace_e>& trace){
            for (Trace_e t : trace) {
                char type = t.type;
                unsigned long address = t.address;

                access(type, address);
            }
        }

        void print_results(){
            hit_rate = (static_cast<double>(hits) / static_cast<double>(total_access)) * 100.0;
            cout << " " << setprecision(3) << fixed << hit_rate << "%" <<  " - " << name << endl;
            return;
        }

        void execute(const vector<Trace_e>& trace){
            run_sim(trace);
            print_results();
        } 
};

class FullAssocTrue : public DirectMap{
    protected:
        unsigned long clock;
        unordered_map <unsigned long, int> tag_map;
    public: 
        FullAssocTrue(){}

        FullAssocTrue(unsigned long cache_size) : DirectMap(cache_size){
            this->clock = 0;
            this->tag_map.reserve(cache_size / 32);
            name = "fully assoc true-LRU";

        }

        unsigned long get_tag(unsigned long address) override{
            return address >> offset_bits;
        }

        void access(char type, unsigned long address) override{
            //no need to calc index
            unsigned long tag = get_tag(address);
            clock++;
            
            if(tag_map.count(tag) > 0){
                hits++;
                table[tag_map[tag]].age = clock;
            }
            //if not found 
            else{
                int min_age = clock;
                int min_index = -1;
                bool is_full = true;

                //check for space if you can add break, if not continue to find the oldest
                for(int i = 0; i < table.size(); i++){
                    if(!table[i].valid){
                        table[i].valid = true;
                        table[i].tag = tag;
                        table[i].age = clock;
                        tag_map[tag] = i;
                        is_full = false;
                        break;
                    }
                }
                    
                    if(is_full){
                        for(int i = 0; i < table.size(); i++){
                            if(table[i].age < min_age){
                                min_age = table[i].age;
                                min_index = i;
                            }
                        }

                        unsigned long old_tag = table[min_index].tag;

                        //replace oldeest and update hashmap
                        table[min_index].tag = tag;
                        table[min_index].age = clock;
                        table[min_index].valid = true;
                        tag_map.erase(old_tag);
                        tag_map[tag] = min_index;

                    }
            }
            total_access++;
        
        }
};


class FullAssocPsudeo : public DirectMap{
    protected:
        vector <bool> bitTree;
        unordered_map <unsigned long, int> tag_map;
        int numLines;

    public:

    FullAssocPsudeo(){}
    FullAssocPsudeo(unsigned long cache_size) : DirectMap(cache_size){
        this->name = "fully assoc psuedo-LRU";
        //this->tag_map.reserve(cache_size / 32);

        this->numLines = cache_size / 32;
        this->tag_map.reserve(cache_size / 32);

        bitTree.resize(511, 0);
    }

    unsigned long get_tag(unsigned long address) override{
        return address >> offset_bits;
    }

    //checked
    int get_victim() {
        int node = 0;
        int counter = 0;


        for (int i = 0; i < 9; i++){
            if (bitTree[node] == 0){
                node = 2 * node + 2;
                counter = (counter << 1) | 0b0;
            }
            else{
                node = 2 * node + 1;
                counter = (counter << 1) | 0b1;
            }
        }
        return ((numLines - 1) - counter);
    }
    void update_path(int index) {
        int node = 0;
        //flipping the index gives the path
        int path = 511 - index;

        for (int i = 0; i < 9; i ++){
            //we want the MSB to LSB
            //int direction = (path >> i) & 1;
            int direction = (path >> (8 - i)) & 0b1;
            int update = (index >> (8 - i)) & 0b1;
            
            //0 means left was cold so we went right, move to right child, set it to 1
            if(direction == 0){
                bitTree[node] = update;
                node = 2 * node + 2;
                
            }
            //so the bit was 1 so right was hot and we went left, move to left child, set it to 1
            else{
                bitTree[node] = update;
                node = 2 * node + 1;
            }
        }
        return;
    
    }
    
    void access(char type, unsigned long address) override{
        unsigned long tag = get_tag(address);


        if(tag_map.count(tag) > 0){
            hits++;
            int index = tag_map[tag];
            update_path(index);
            total_access++;
            return;
        }
        int victim_index = 0;
        bool is_full = true;

        victim_index = get_victim();

        unsigned long old_tag = table[victim_index].tag;

        table[victim_index].tag = tag;
        table[victim_index].valid = true;
        update_path(victim_index);
        tag_map.erase(old_tag);
        tag_map[tag] = victim_index;

      
        total_access++;
        return;
    }
};

//allows to use clock
class SetAssoc : public FullAssocTrue{

    protected:
        int num_sets;
        int assoc;
        vector <vector <Cache_line>> setTable;
        bool miss;
    public:
        SetAssoc(){}
    
        SetAssoc(unsigned long cache_size, int assoc) : FullAssocTrue(cache_size){
            this->assoc = assoc;
            int num_lines = cache_size / 32;
            this->num_sets = num_lines / assoc;

            if (assoc == 2){
                name = "set assoc 2";
            }else if(assoc == 4){
                name = "set assoc 4";
            }else if(assoc == 8){
                name = "set assoc 8";
            }else if(assoc == 16){
                name = "set assoc 16";
            }

            setTable.resize(num_sets, vector<Cache_line>(assoc));

        }

        unsigned long get_index(unsigned long address)override{
            return (address >> offset_bits) % num_sets;
        }

        void access(char type, unsigned long address) override{
            
            unsigned long index = get_index(address);
            unsigned long tag = get_tag(address);
            clock++;

            //loop through the set to see if the tag is there update age if so 
            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].valid && setTable[index][i].tag == tag){
                    hits++;
                    setTable[index][i].age = clock;
                    total_access++;
                    miss = false;
                    return;
                }
            }

            miss = true;
            //since we already got to this point we know its a miss
            //miss checking 
            for(int i = 0; i < assoc; i++){
                if(!setTable[index][i].valid){
                    setTable[index][i].valid = true;
                    setTable[index][i].tag = tag;
                    setTable[index][i].age = clock;
                    total_access++;
                    return;
                }
            }

            //the table is full so we need to evict
            int min_age = clock;
            int min_index = 0;

            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].age < min_age){
                    min_age = setTable[index][i].age;
                    min_index = i;
                }
            }

            setTable[index][min_index].tag = tag;
            setTable[index][min_index].age = clock;
            setTable[index][min_index].valid = true;

            total_access++;
            return;
        }
};

class SetAssocSkipWrite : public SetAssoc{
    public:
        SetAssocSkipWrite(){}
        SetAssocSkipWrite(unsigned long cache_size, int assoc) : SetAssoc(cache_size, assoc){
            if(assoc == 2){
                name = "set assoc 2 skip on write miss";
            }else if(assoc == 4){
                name = "set assoc 4 skip on write miss";
            }else if(assoc == 8){
                name = "set assoc 8 skip on write miss";
            }else if(assoc == 16){
                name = "set assoc 16 skip on write miss";
            }
        }

        //same code just with skip write
        void access(char type, unsigned long address) override{
            unsigned long index = get_index(address);
            unsigned long tag = get_tag(address);
            clock++;

            //loop through the set to see if the tag is there update age if so 
            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].valid && setTable[index][i].tag == tag){
                    hits++;
                    setTable[index][i].age = clock;
                    total_access++;
                    return;
                }
            }
            if (type == 'S'){
                total_access++;
                return; //skip write
            }  

            //since we already got to this point we know its a miss
            //miss checking 
            for(int i = 0; i < assoc; i++){
                if(!setTable[index][i].valid){
                    setTable[index][i].valid = true;
                    setTable[index][i].tag = tag;
                    setTable[index][i].age = clock;
                    total_access++;
                    return;
                }
            }

            //the table is full so we need to evict
            int min_age = clock;
            int min_index = 0;

            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].age < min_age){
                    min_age = setTable[index][i].age;
                    min_index = i;
                }
            }

            setTable[index][min_index].tag = tag;
            setTable[index][min_index].age = clock;
            setTable[index][min_index].valid = true;

            total_access++;
            return;
        }
};

class SetAssocPrefetch : public SetAssoc{
    public:
        SetAssocPrefetch(){}
        SetAssocPrefetch(unsigned long cache_size, int assoc) : SetAssoc(cache_size, assoc){
            if(assoc == 2){
                name = "set assoc 2 prefetch";
            }else if(assoc == 4){
                name = "set assoc 4 prefetch";
            }else if(assoc == 8){
                name = "set assoc 8 prefetch";
            }else if(assoc == 16){
                name = "set assoc 16 prefetch";
            }
        }

        void access(char type, unsigned long address) override{
            
            SetAssoc::access(type, address);

            unsigned long index = get_index(address + 32);
            unsigned long tag = get_tag(address + 32);

            //loop through the set to see if the tag is there update age if so 
            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].valid && setTable[index][i].tag == tag){
                    setTable[index][i].age = clock;
                    return;
                }
            }

            //since we already got to this point we know its a miss
            //miss checking 
            for(int i = 0; i < assoc; i++){
                if(!setTable[index][i].valid){
                    setTable[index][i].valid = true;
                    setTable[index][i].tag = tag;
                    setTable[index][i].age = clock;
                    return;
                }
            }

            //the table is full so we need to evict
            int min_age = clock;
            int min_index = 0;

            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].age < min_age){
                    min_age = setTable[index][i].age;
                    min_index = i;
                }
            }

            setTable[index][min_index].tag = tag;
            setTable[index][min_index].age = clock;
            setTable[index][min_index].valid = true;
            return;
        }

    };

class SetAssocPrefetchOnMiss : public SetAssoc{
    public:
    SetAssocPrefetchOnMiss(){}
    SetAssocPrefetchOnMiss(unsigned long cache_size, int assoc) : SetAssoc(cache_size, assoc){
        if(assoc == 2){
            name = "set assoc 2 prefetch on miss";
        }else if(assoc == 4){
            name = "set assoc 4 prefetch on miss";
        }else if(assoc == 8){
            name = "set assoc 8 prefetch on miss";
        }else if(assoc == 16){
            name = "set assoc 16 prefetch on miss";
        }
    }

    void access(char type, unsigned long address) override{
        
        SetAssoc::access(type, address);

        if (miss){
            unsigned long index = get_index(address + 32);
            unsigned long tag = get_tag(address + 32);

            //loop through the set to see if the tag is there update age if so 
            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].valid && setTable[index][i].tag == tag){
                    setTable[index][i].age = clock;
                    return;
                }
            }

            //since we already got to this point we know its a miss
            //miss checking 
            for(int i = 0; i < assoc; i++){
                if(!setTable[index][i].valid){
                    setTable[index][i].valid = true;
                    setTable[index][i].tag = tag;
                    setTable[index][i].age = clock;
                    return;
                }
            }

            //the table is full so we need to evict
            int min_age = clock;
            int min_index = 0;

            for(int i = 0; i < assoc; i++){
                if(setTable[index][i].age < min_age){
                    min_age = setTable[index][i].age;
                    min_index = i;
                }
            }

            setTable[index][min_index].tag = tag;
            setTable[index][min_index].age = clock;
            setTable[index][min_index].valid = true;
        }
        return;
    }
};

//read all traces into vector similar to proj1
void read_trace (vector<Trace_e>&trace){

    string line;
    while (getline(cin, line)) {
        stringstream s(line);

        unsigned long long address;;
        char type;

        s >> type >> hex >> address;
        trace.push_back({type, address});
    }
    return;
}

int main(int argc, char *argv[]){
    //read the trace file into a vector 
    vector <Trace_e> trace;
    read_trace(trace);
    double best_hit = 0;

    DirectMap* list[] = {
        new DirectMap(1024), 
        new DirectMap(4096),
        new DirectMap(16384),
        new DirectMap(32768),
        new FullAssocTrue(16384),
        new FullAssocPsudeo(16384),
        new SetAssoc(16384, 2),
        new SetAssoc(16384, 4),
        new SetAssoc(16384, 8),
        new SetAssoc(16384, 16),
        new SetAssocSkipWrite(16384, 2),
        new SetAssocSkipWrite(16384, 4),
        new SetAssocSkipWrite(16384, 8),
        new SetAssocSkipWrite(16384, 16),
        new SetAssocPrefetch(16384, 2),
        new SetAssocPrefetch(16384, 4),
        new SetAssocPrefetch(16384, 8),
        new SetAssocPrefetch(16384, 16),
        new SetAssocPrefetchOnMiss(16384, 2),
        new SetAssocPrefetchOnMiss(16384, 4),
        new SetAssocPrefetchOnMiss(16384, 8),
        new SetAssocPrefetchOnMiss(16384, 16)
    };

    //add a best hit rate tracker
    for(auto* csim : list){
        csim->execute(trace);
        if(csim->hit_rate > best_hit){
            best_hit = csim->hit_rate;
        }
        delete csim;
    };
    cout << "Best hit rate: " << best_hit << "%" << endl;
    return 0;
}