#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <climits>
#include <stack>
#include <queue>
using namespace std;

class Word{
	public:
		int num;
		int len;
		int startX, startY;
		char direction;
		Word(){};
		Word(int x, int y, int l, char d){
			startX = x;
			startY = y;
			len = l;
			direction = d;
		};
		// for hash key
		bool operator==(const Word &other) const
  		{ return (len == other.len
            && startX == other.startX
            && startY == other.startY
            && direction== other.direction);
 		}
};
namespace std {

  template <>
  struct hash<Word>
  {
    std::size_t operator()(const Word& k) const
    {
      using std::size_t;
      using std::hash;

      // Compute individual hash values 
      return (hash<int>()(k.startX) ^ 
	  			((hash<int>()(k.startY) ^ 
					((hash<int>()(k.len) ^ 
						(hash<char>()(k.direction) << 1)) << 1)) <<1));    }
  };

}
class Constraint{//Binary constraint
	public:
		Word from;
		Word to;
		int from_index;
		int to_index;
		Constraint(){};
		Constraint(Word a, Word b, int a_index, int b_index){
			this->from = a;
			this->to = b;
			this->from_index = a_index;
			this->to_index = b_index;
		};
};
vector<string> vocabulary[15];//the length of the longest vocabulary
unordered_map<Word, string > assignments;
stack< pair<Word,string> > assign_stack;
int node_expand =0;
int answer_cnt = 0;

class Puzzle{
	public:
		vector<Word> words;//Variables
		unordered_map<Word, vector<string> > domains;//use to store domain of word
		unordered_map<Word, vector<string> > old_domains;//use to reset domain to ussign state
		unordered_map<Word, unordered_map<Word, vector<string> > > old_state; //use to reset other domain to ussign state
		unordered_map<Word, vector<Constraint> > neighbors;
		
		// Constructor
		Puzzle(){};
		Puzzle(string str){
			stringstream ss(str);
			int startX, startY, len;
			char direction;
			while(ss>>startX>>startY>>len>>direction){
				Word word( startX,startY,len,direction);
				this->words.push_back(word);
			}
		}
		// set domain with corresponding vocabulary 
		void setDomain(){
			for(Word& word: this->words){
				this->domains[word] = vocabulary[word.len];
			}

		}
		// set neighbors of all words
		void setConstraint(){
			for(int i = 0; i < this->words.size(); i ++){
				for(int j = 0 ; j < this->words.size() ; j++){
					if( i == j ) continue; //the same word
					if(this->words[i].direction == this->words[j].direction) continue;
					if(this->words[i].direction =='A'){
						if((this->words[j].startX >= this->words[i].startX 
								&& this->words[j].startX <= this->words[i].startX +this->words[i].len - 1)// x accepted
							&& (this->words[i].startY >= this->words[j].startY 
								&& this->words[i].startY <= this->words[j].startY + this->words[j].len - 1)){// y accepted
								int index_from = this->words[j].startX - this->words[i].startX;
								int index_to = this->words[i].startY - this->words[j].startY;
								Constraint c(this->words[i],this->words[j],index_from,index_to);
								//push neighbors
								this->neighbors[this->words[i]].push_back(c);
						}
					}
					else if(this->words[i].direction =='D'){
						if((this->words[i].startX >= this->words[j].startX 
								&& this->words[i].startX <= this->words[j].startX +this->words[j].len - 1)// x accepted
							&& (this->words[j].startY >= this->words[i].startY
								&& this->words[j].startY <= this->words[i].startY + this->words[i].len - 1)){// y accepted
								int index_from = this->words[j].startY - this->words[i].startY;
								int index_to = this->words[i].startX - this->words[j].startX;
								Constraint c(this->words[i],this->words[j],index_from,index_to);
								//push neighbors
								this->neighbors[this->words[i]].push_back(c);
							}
					}
				}
			}
			
		}
		//AC3 check if remove inconsistent value then push neighbor's neighbor into queue
		bool removeInconsistentValue(Constraint c){
			bool removal = false;
			for(auto i = this->domains[c.to].begin(); i != this->domains[c.to].end();){//Check all possible string of this variable 
				bool someValue= false;
				for(auto j = this->domains[c.from].begin(); j != this->domains[c.from].end();++j){//Compared with static source
					if((*i)[c.to_index]== (*j)[c.from_index])
					{	
						 someValue= true;
					}
				}
				if (someValue) ++i;// this string is consistent 
				else {
					this->domains[c.to].erase(i);
					removal = true;
				}
			}
			return removal;
		}
		// maintain arc consistency
		void AC3(queue<Constraint> q){
			while(!q.empty()){
				Constraint c = q.front(); q.pop();
				// check if remove inconsistent value then push neighbor's neighbor into queue
				if(removeInconsistentValue(c)){
					for(Constraint cst: this->neighbors[c.to]){
						q.push(cst);
					}
				}
			}
		}
		// assign string to word
		bool assign(Word word, string str, bool fc = false,bool ac3 = false){
			assign_stack.push(make_pair(word,str));
			this->old_domains[word] = this->domains[word];
			vector<string> assign_val = {str};
			this->domains[word] = assign_val;
			assignments[word] = str;
			// Apply AC3 or fc
			if (ac3) {
				// store previous state
				this->old_state[word] = this->domains;
				this->old_state[word][word] = this->old_domains[word];
				// push neighbor into queue
				queue<Constraint> q;
				for(Constraint c: this->neighbors[word]) q.push(c);
				// apply ac3
				AC3(q);
				// check whether neighbor is empty
				for(Word word: this->words){
					if(this->domains[word].empty()){
						return false;
					}
				}

			}
			else if(fc){
				// store previous state
				this->old_state[word] = this->domains;
				this->old_state[word][word] = this->old_domains[word];
				// check all negihbors domain, remove unvalid vocabulary
				for(Constraint c: this->neighbors[word]){
					auto got = assignments.find(c.to);
					if(got != assignments.end()) continue;// this neighbor has word but checked already
					for(auto i = this->domains[c.to].begin(); i != this->domains[c.to].end();){// iterate all neighbors domain 
						if((*i)[c.to_index]!= str[c.from_index])//remove illegal ones 
						{	
							this->domains[c.to].erase(i);
						}
						else ++i;
					}
					if(this->domains[c.to].empty()) {	
						return false;
					}
				}
			}
			
			return true;
		}
		// unassign word and recover domain
		void unassign(Word word, bool fc = false, bool ac3 = false){
			assign_stack.pop();
			auto got = assignments.find(word);
			if(got == assignments.end()) return;
			assignments.erase(word);
			//reset domain of unassign word
			if(fc||ac3) this->domains = this->old_state[word];
			else this->domains[word] = this->old_domains[word];
			//printf("Unassign Successfully\n");

		}
		// select next word to be assigned
		Word selectUnsignedWord(bool MRV = false, bool DEGREE = false){
			if (MRV && DEGREE){// apply mrv first and then degree
				Word return_word;
				int least_domain_num = INT_MAX;
				int most_degree_cnt = 0;
				for(int i = 0; i <this->words.size(); i++){
					auto got = assignments.find(this->words[i]);
					if(got == assignments.end() && this->domains[this->words[i]].size() < least_domain_num){
						return_word = this->words[i];
						least_domain_num = this->domains[this->words[i]].size();
					}
					else if(got == assignments.end() && this->domains[this->words[i]].size() == least_domain_num){
						if(this->neighbors[this->words[i]].size() > most_degree_cnt){
							return_word = this->words[i];
							most_degree_cnt = this->domains[this->words[i]].size();
						}
					}

				}
				return return_word;
			}
			else if (DEGREE){// find the word which has the most neighbors
				Word degree_max_word;
				int most_degree_cnt = 0;
				for(int i = 0; i <this->words.size(); i++){
					auto got = assignments.find(this->words[i]);
					if(got == assignments.end() && this->neighbors[this->words[i]].size() > most_degree_cnt){
						degree_max_word = this->words[i];
						most_degree_cnt = this->neighbors[this->words[i]].size();
					}
				}
				return degree_max_word;
			}

			else if (MRV){//find the word which has the minimum remaining value
				Word mini_remain_word;
				int least_domain_num = INT_MAX;
				for(int i = 0; i <this->words.size(); i++){
					auto got = assignments.find(this->words[i]);
					if(got == assignments.end() && this->domains[this->words[i]].size() <= least_domain_num){
						mini_remain_word = this->words[i];
						least_domain_num = this->domains[this->words[i]].size();
					}
				}
				return mini_remain_word;
			}
			else{// choose the first found word
				for(int i = 0; i <this->words.size(); i++){
					auto got = assignments.find(this->words[i]);
					if(got == assignments.end()){
						return this->words[i];
					}
				}
			}
		}
		//check this vocabulary dpesn't conflict
		bool nonConflict(Word select, string str){
			for(Constraint& c :this->neighbors[select]){
				auto got = assignments.find(c.to);
				if(got == assignments.end()) {}// this neighbor doesn't have value yet
				else if(str[c.from_index] != assignments[c.to][c.to_index])// different char in the same position
					return false;
			}
			return true;
		}
			

};


bool recursiveBackTracking(Puzzle& P,bool FC = false, bool MRV = false, bool DEGREE = false, bool show_all = false, bool ac3 = false){
	if (assignments.size() == P.words.size()) return true;// all word are assigned
	else{	
		Word select = P.selectUnsignedWord(MRV, DEGREE);// select next word
		for(string str: P.domains[select]){// iterate all domain of that word
			if(P.nonConflict(select, str)) { 
				if (!P.assign(select,str,FC,ac3)){//check fc or ac3
					P.unassign(select,FC,ac3);
					continue;
				}
				node_expand++;
				bool result = recursiveBackTracking(P, FC, MRV, DEGREE, show_all,ac3);
				if(result && !show_all) return result;
				else if (result && show_all) {//for find all answer
					answer_cnt++;
				}
				P.unassign(select,FC,ac3);

			}
		}
		
		return false;
	}
}
void backTracking (Puzzle& P,bool FC = false, bool MRV = false, bool DEGREE = false, bool show_all = false, bool ac3 = false){
	if(recursiveBackTracking(P, FC, MRV, DEGREE, show_all, ac3)){
		while(!assign_stack.empty()){// print answer
			pair<Word,string> stk_top = assign_stack.top();
			cout <<"Word x: " << stk_top.first.startX<<", y: "<<stk_top.first.startY<<", len: "\
				<<stk_top.first.len<<", direction: "<<stk_top.first.direction\
				<< ", is filled with : "<< stk_top.second <<endl;
			assign_stack.pop();
		}
		printf("node expand: %d\n", node_expand);

	}
	else if (show_all)  printf("answer count: %d\n", answer_cnt);// print the number of all answer
	else { printf("Answer Not Found\n");}// no answer
}

int main(){
	bool FC = false;  // Apply forward checking
	bool MRV = false; // Apply Minimum Remaining Value selection
	bool DEGREE = false;// Apply most neighbors Variable selection
	bool show_all = false;// Find an answer and stop or go through all possible answers
	bool ac3 = true; // Apply ac3
	ifstream fin2("English Words 3000.txt");
	ifstream fin("puzzle.txt");
	if (!fin||!fin2) { // Open file failed 
		cout << "Error opening file. Shutting down..." << endl;
		return 0;
	}
	for(string str; getline( fin2, str ,'\n'); ){ // store all vocabulary in vector
		stringstream ss(str); // remove weird characters
		string str2;
		ss>>str2;
		vocabulary[str2.size()].push_back(str2);
	}
	int i = 1;
	for(string str; getline( fin, str ,'\n'); ){
		Puzzle P(str); // initial puzzle with input string
		P.setDomain();	// set domain initially with corresponding length of vocabulary
		P.setConstraint(); // set neighbors of all words
		printf("-------------Puzzle %d--------------\n", i++);
		backTracking( P, FC, MRV, DEGREE, show_all,ac3); 
		// reset variables
		node_expand = 0;
		answer_cnt = 0;
		str = "";
		assignments = {};
	}
	fin.close();
	fin2.close();
	return 0;
}
