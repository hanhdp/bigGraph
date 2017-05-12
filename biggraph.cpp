#include <string.h>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <tuple>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <vector>
#include <tuple>
#include <stdint.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <cilk/cilk_api.h>
#include <cilk/cilk.h>
//#include "tbb/concurrent_vector.h" 
#define OUTGOING 0
#define INCOMING 1

using namespace std;

template <typename T> class VQueue {
	vector<T> data;
	size_t V;
	size_t curr_;
	size_t next_;
	size_t end_;

public:
	explicit VQueue(size_t V) : data(V), V(V), curr_(0), next_(0), end_(0){
	}
  // virtual ~VQueue(){ delete [] data; }
	inline bool empty() const { return curr_ == next_; }
	inline bool full() const { return end_ == V; }
	inline T &front() { return data[curr_];}
	inline size_t size() const { return end_; }
	inline void pop() { ++curr_; assert(curr_ <= end_);}
	inline void push(const T &val){ data[end_++] = val; assert(end_ <= V);}
	inline void next() { assert(curr_ == next_); next_ = end_; }
	inline void clear() { curr_ = next_ = end_ = 0; }
	inline void resize(size_t V_){
		if (V_ > V){ V = V_; data.resize(V); }
	}

	inline typename vector<T>::iterator begin() { return data.begin();}
	inline typename vector<T>::iterator end() { return data.begin() + end_;}
};

const uint32_t ALIVE   = 0;
const uint32_t DEAD    = 1;
const uint32_t UNKNOWN = 2;
const uint32_t MASK    = 3;

//tbb::concurrent_
vector<tuple<uint32_t, uint32_t, uint32_t, char> > 	updates; // <source, target, timestamp, type>
vector<tuple<uint32_t, uint32_t, uint32_t> > 		queries; // <source, target, timestamp>
vector<vector<uint32_t> > 							Edges[2];//outEdges -inEdges, ;
vector<vector<uint32_t> > 							Maps;//for all threads (in+out)
vector<vector<VQueue<uint32_t> > > 					Queues;//for all threads (in+out)
vector<uint32_t> 									S[2];
uint32_t  											Node_Num = 0;
uint32_t num_threads = __cilkrts_get_nworkers();

static inline uint32_t GetID(uint32_t v) { return v >> 2; }
static inline uint32_t GetState(uint32_t v)  { return v & MASK; }
static inline uint32_t ToEdge(uint32_t v) { return (v << 2) ;} //| ALIVE
static inline void ResetMap(uint32_t threadID)
{
	for (uint32_t dir = 0; dir < 2; dir++) {
		for (uint32_t v : Queues[threadID][dir]) {
			Maps[threadID][v >> 4] = 0;
		}
		Queues[threadID][dir].clear();
	}
}
//set bit at v in Maps
static inline void SetBit(uint32_t threadID, uint32_t v, uint32_t direction)
{
	Maps[threadID][v >> 4] |= 1 << (direction + ((v & 15)<< 1) );
}

//test bit at v in Maps
static inline int TestBit(uint32_t threadID, uint32_t v, uint32_t direction)
{
	unsigned p = ((v & 15)<<1) + direction;
	return (Maps[threadID][v >> 4] >> p) & 1;
}

inline bool readeof() {
	for (;;) {
		int c = getc_unlocked(stdin);
		if (c == EOF || c == '#') {
			return true;
		} else if (isspace(c)) {
			continue;
		} else {
			ungetc(c, stdin);
			return false;
		}
	}
	assert(false);
}

inline uint32_t readuint() {
	uint32_t c;
	uint32_t x;
	while (!isdigit(c = getc_unlocked(stdin)));
	x = c - '0';
	while (isdigit(c = getc_unlocked(stdin))) {
		x = (x * 10 + (c - '0'));
	}
	return x;
}
//check for timestamp
bool inline IsEdgeAlive(uint32_t u, uint32_t v, uint32_t time, uint32_t state){

	auto iter = lower_bound(updates.begin(), updates.end(), make_tuple(u, v, time, 0));
	if (updates.empty() || iter == updates.begin()){
		return (state & 1) == 0;
	}

	auto last = *(--iter);
	if (u == get<0>(last) && v == get<1>(last)){
		return get<3>(last) == 'A';
	} else {
		return (state & 1) == 0;
	}
}
int QueryDistance(uint32_t qIndex)
{
	uint32_t s, t, time;
	tie(time, s,t) = queries[qIndex];

	if (s == t) return 0;

	int dist_ub =  1e2;

	uint32_t threadID = __cilkrts_get_worker_number();    
	auto &Q = Queues[threadID];
	int res = -1, distance[2] = {0, 0};
	int weight[2] = {0, 0}, v;

	for (int dir = 0; dir < 2; dir++){
		v = dir == 0 ? s : t;
		Q[dir].clear();
		Q[dir].push(v);
		Q[dir].next();
		SetBit(threadID, v, dir);
		//D[v >> 4] |= 1 << (dir + ((v & 15) << 1 ) );
		weight[dir] += Edges[dir][v].size();
	}

	auto bfs_step = [&](int direction) -> bool{
		distance[direction]++;
		const auto & G_ = Edges[direction];
		const auto & S_ = S[direction];
		auto &       Q_ = Q[direction];
		weight[direction] = 0;

		while (!Q_.empty()) {
			uint32_t v = Q_.front();
  			Q_.pop();

			for (uint32_t w_ : G_[v]) {
  				uint32_t w = GetID(w_);
  				// auto & d = D[w >> 4];	const int bit_pos = ((w & 15) << 1 ) + direction;
    			// if (d & (1 << bit_pos)) continue;
    			if (TestBit(threadID,w, direction)) continue;
				uint32_t state = GetState(w_);
				if (state == ALIVE || ((state & UNKNOWN) &&
					(direction == 0 ? IsEdgeAlive(v, w, time, state) :
						IsEdgeAlive(w, v, time, state) ) ) ){
					//if (d & (1 << (1 - 2 * direction + bit_pos))) {
					if (TestBit(threadID,w, 1-direction)) {
						res = distance[0] + distance[1];
  						return true;
					} else {
						Q_.push(w);
						//d |= 1 << bit_pos;
						SetBit(threadID,w, direction);
					}
				}
			}
			weight[direction] += S_[v];			
			//weight[direction] += G_[v].size();
		}
		Q_.next();    
		return false;
	};


	for (int direction = 0; direction < 2; ++direction) {
		if(bfs_step(direction)) goto LOOP_END;
	}

	while (!Q[OUTGOING].empty() && !Q[INCOMING].empty()) {
		const int direction = (weight[0] <= weight[1]) ? 0 : 1;
		if (distance[0] + distance[1] + 1 == dist_ub){
			res = dist_ub;
			goto LOOP_END;
		}
		if(bfs_step(direction)) goto LOOP_END;
	}

	LOOP_END:
	ResetMap(threadID);

	return res;
}

inline void InsertNode(vector<uint32_t> &vs, uint32_t v){//mark this node is being added
	auto iter = lower_bound(vs.begin(), vs.end(), ToEdge(v));
	if (iter != vs.end() && GetID(*iter) == v){//v exists on list vs
		if (GetState(*iter) != ALIVE) *iter |= UNKNOWN;		
	} else {// not exist
		vs.insert(iter, ToEdge(v)  | MASK);
	}
}
inline void InsertEdge(vector<uint32_t> &vs, uint32_t v){//real add
	auto iter = lower_bound(vs.begin(), vs.end(), ToEdge(v));
	if (iter != vs.end() && GetID(*iter) == v){//v exists on list vs
		*iter = ToEdge(v);// | ALIVE;
	}
}

inline void DeleteNode(vector<uint32_t> &vs, uint32_t v){//mark this node is being deleted
	auto iter = lower_bound(vs.begin(), vs.end(), ToEdge(v));
    if (iter != vs.end() && GetID(*iter) == v){//if found
    	if (GetState(*iter) != DEAD) *iter |= UNKNOWN;
    }
} 
inline void DeleteEdge(vector<uint32_t> &vs, uint32_t v){//real delete
	auto iter = lower_bound(vs.begin(), vs.end(), ToEdge(v));
    if (iter != vs.end() && GetID(*iter) == v){//if found
    		*iter = ToEdge(v) | DEAD;
    }
} 
void Build()
{
	cerr << "Building the big graph..." ;
    //1.fetch edge set from stdin  
	FILE *fp = stdin; size_t bufsize=0;char *line = NULL;
    int res; uint32_t u, v;
	Edges[0].clear(); Edges[1].clear();
	Edges[0].resize(1e7); Edges[1].resize(1e7);   
    // vertex identified from 0 -> n
	while (true){
		res = getline(&line,&bufsize,fp);
		if (res == -1) break;
		if (line[0] == 'S') break;

		res = sscanf(line, "%u %u", &u, &v);
		if ( !res || res == EOF ) {
			continue;
		} 
		Node_Num = max({Node_Num, u + 1, v + 1});  
		if (Node_Num>1e7-1){
			Edges[0].resize(1e8); Edges[1].resize(1e8);  
		}
		Edges[0][u].push_back(ToEdge(v));      
		Edges[1][v].push_back(ToEdge(u));
	}
	//cerr << "End of Read" << endl;

    //sort adjacent lists
	cilk_for (uint32_t v = 0; v < Node_Num; v++){
		sort(Edges[0][v].begin(), Edges[0][v].end());    
		sort(Edges[1][v].begin(), Edges[1][v].end());
	}

    Node_Num += 1e5;//add more nodes 
    //cerr << "End of init" << endl;
    Edges[0].resize(Node_Num);
    Edges[1].resize(Node_Num);


    //2. Init the graph
    Queues.clear();
    Maps.clear();
    
    for (uint32_t t = 0; t < num_threads; t++){
    	Queues.emplace_back(2, VQueue<uint32_t>(Node_Num));
    	Maps.emplace_back(Node_Num / (sizeof(Maps[t][0]) * 4));
      //cerr << (Maps[t][0]) << " " << Node_Num / (sizeof(Maps[t][0]) * 4) << endl;
    }

    cilk_for (int dir = 0; dir < 2; dir++){
    	S[dir].resize(Node_Num);
    	for (uint32_t v = 0; v < Node_Num; v++){
    		for (uint32_t w : Edges[dir][v]){
    			S[dir][v] += Edges[dir][GetID(w)].size();
    		}
    	}
    }
    // cerr << "Num of nodes " << Node_Num << endl;
    cerr << ". Done!" << endl;
}


vector<int> ProcessBatch()
{
	sort(updates.begin(), updates.end());
	//sort(queries.begin(), queries.end());
	uint32_t max_thread = num_threads;
	//for outgoing nodes
    cilk_for (uint32_t t = 0; t < max_thread; t++){
		for (size_t i = 0; i < updates.size(); i++){
			if (get<0>(updates[i]) % max_thread == t){				
				char cmd; uint32_t u, v, time, V;
				tie(u, v, time, cmd) = updates[i];
				V = max(u, v) + 1;
				if (Edges[0].size() < V){//node ID is bigger than Edges size ==> allocate more nodes
					Edges[0].resize(V);
					Edges[1].resize(V);
					S[0].resize(V);S[1].resize(V);
					
					cilk_for (uint32_t t = 0; t < num_threads; t++){
						Maps[t].resize(V / (sizeof(Maps[t][0]) * 4) + 1);
						Queues[t][0].resize(V);
						Queues[t][1].resize(V);
					}
				}
				if (cmd == 'A'){
					InsertNode(Edges[OUTGOING][u], v);
					//S[OUTGOING][u] += Edges[OUTGOING][(v)].size();		    		
				}else{
					DeleteNode(Edges[OUTGOING][u], v);
					//S[OUTGOING][u] -= Edges[OUTGOING][(v)].size();	
				}
			}			
		}
	}
	//for incoming nodes
	cilk_for (uint32_t t = 0; t < max_thread; t++){
		for (size_t i = 0; i < updates.size(); i++){			
			if (get<1>(updates[i]) % max_thread == t){
				char cmd; uint32_t u, v, time;
				tie(u, v, time, cmd) = updates[i];
				if (cmd == 'A'){
					InsertNode(Edges[INCOMING][v], u);
					//S[INCOMING][v] += Edges[INCOMING][u].size();
				}
				else{
					DeleteNode(Edges[INCOMING][v], u);
					//S[INCOMING][v] -= Edges[INCOMING][u].size();
				}
			}			
		}
	}
    
	//compute shortest distance
	vector<int> res(queries.size());
	cilk_for (size_t i = 0; i < queries.size(); i++){
      	res[i] = QueryDistance(i);
	}
	//real updates
	cilk_for (uint32_t i=0; i< updates.size(); i++){
		char cmd; uint32_t u, v, t;
		tie(u, v, t, cmd) = updates[i];
		if (cmd == 'A'){
			InsertEdge(Edges[0][u], v);
			S[OUTGOING][u] += Edges[OUTGOING][(v)].size();
			InsertEdge(Edges[1][v], u);
			S[INCOMING][v] += Edges[INCOMING][u].size();
		}else{
			DeleteEdge(Edges[0][u], v);
			S[OUTGOING][u] -= Edges[OUTGOING][(v)].size();
			DeleteEdge(Edges[1][v], u);
			S[INCOMING][v] -= Edges[INCOMING][u].size();
		}
	}
	queries.clear();
	updates.clear();

	return res;
}

int main(int argc, char *argv[])
{
	__cilkrts_set_param("nworkers",to_string(num_threads).data());
	cerr << "NumThread = " << num_threads << endl;
	Build();

	queries.reserve(10000);

  //warm memory
	cerr << "Warming Cache ...";
	for (int i = 0; i < 10000; ++i) {
		queries.emplace_back('Q', rand() % Node_Num, rand() % Node_Num);
	}
	ProcessBatch();

 	cerr << ". Done !\n"; 
	puts("R");
	cout.flush();
 
	vector<int> dists;
	char cmd; uint32_t u, v, n;
	//char num[6];

	while (true){
		if(readeof()) break;    
		n=0;
		while ((cmd = getc_unlocked(stdin)) && cmd != 'F'){
			u = readuint();
			v = readuint();
			if (cmd=='Q') queries.push_back(make_tuple(n, u, v));
			else if(u != v) updates.push_back(make_tuple(u, v, n, cmd));
			++n;
		}
		//end of input for batch, start to perform batch
		dists = ProcessBatch();
		string out;
		for (auto d : dists) {        
			//sprintf(num, "%d\n",  d);			out += num;
			out += to_string(d) + "\n";
		}
		fputs_unlocked(out.c_str(), stdout);
		fflush_unlocked(stdout);
	}

	return 0;
}
