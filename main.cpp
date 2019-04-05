#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <climits>
#include <stack>
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
			printf("word: x = %d, y = %d, l = %d, d = %c\n",startX,startY,len,direction);
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
      using std::string;

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

vector<string> vocabulary[15];								//the length of the longest vocabulary
unordered_map<Word, string > assignments;
stack<Word> assign_stack;
int node_expand =0;
int answer_cnt = 0;
class Puzzle{
	public:
		vector<Word> words;//Variables
		unordered_map<Word, vector<string> > domains;
		unordered_map<Word, vector<string> > old_domains;//use to reset domain to ussign state
		unordered_map<Word, unordered_map<Word, vector<string> > > fc_state; //use to reset other domain to ussign state
		unordered_map<Word, vector<Constraint> > neighbors;
		

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
		void setConstraint(){
			for(int i = 0; i < this->words.size(); i ++){
				for(int j = 0 ; j < this->words.size() ; j++){
					if( i == j ) continue; //the same word
					if(this->words[i].direction == this->words[j].direction) continue;
					if(this->words[i].direction =='A'){
						if((this->words[j].startX >= this->words[i].startX && this->words[j].startX <= this->words[i].startX +this->words[i].len - 1)// x accepted
							&& (this->words[i].startY >= this->words[j].startY && this->words[i].startY <= this->words[j].startY + this->words[j].len - 1)){// y accepted
								int index_from = this->words[j].startX - this->words[i].startX;
								int index_to = this->words[i].startY - this->words[j].startY;
								Constraint c(this->words[i],this->words[j],index_from,index_to);
								this->neighbors[this->words[i]].push_back(c);
						}
					}
					else if(this->words[i].direction =='D'){
						if((this->words[i].startX >= this->words[j].startX && this->words[i].startX <= this->words[j].startX +this->words[j].len - 1)// x accepted
							&& (this->words[j].startY >= this->words[i].startY && this->words[j].startY <= this->words[i].startY + this->words[i].len - 1)){// y accepted
								int index_from = this->words[j].startY - this->words[i].startY;
								int index_to = this->words[i].startX - this->words[j].startX;
								Constraint c(this->words[i],this->words[j],index_from,index_to);
								this->neighbors[this->words[i]].push_back(c);
							}
					}
				}
			}
			
		}
		bool assign(Word word, string str, bool fc = false){
			this->old_domains[word] = this->domains[word];
			vector<string> assign_val = {str};
			this->domains[word] = assign_val;
			assignments[word] = str;
			//cout<<"Assign " <<str<< " Successfully"<<endl;
			if(fc){
				this->fc_state[word] = this->domains;
				this->fc_state[word][word] = this->old_domains[word];
				for(Constraint c: this->neighbors[word]){
					auto got = assignments.find(c.to);
					if(got != assignments.end()) continue;// this neighbor has word but checked already
					for(auto i = this->domains[c.to].begin(); i != this->domains[c.to].end();){//iterate all neighbors domain 
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

		void unassign(Word word, bool fc = false){
			auto got = assignments.find(word);
			if(got == assignments.end()) return;
			assignments.erase(word);
			//reset domain of unassign word
			if(fc) this->domains = this->fc_state[word];
			else this->domains[word] = this->old_domains[word];
			//printf("Unassign Successfully\n");

		}
		
		Word selectUnsignedWord(bool MRV = false, bool DEGREE = false){
			if (MRV && DEGREE){//apply mrv first
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
							least_domain_num = this->domains[this->words[i]].size();
						}
					}

				}
				return return_word;
			}
			else if (DEGREE){
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

			else if (MRV){

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
			else{
				for(int i = 0; i <this->words.size(); i++){
					auto got = assignments.find(this->words[i]);
					if(got == assignments.end()){
						return this->words[i];
					}
				}
			}
		}
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
bool recursiveBackTracking(Puzzle& P,bool FC = false, bool MRV = false, bool DEGREE = false, bool show_all = false){
	if (assignments.size() == P.words.size()) return true;
	else{	
		Word select = P.selectUnsignedWord(MRV, DEGREE);
		for(string str: P.domains[select]){
			if(P.nonConflict(select, str)) {
				if (!P.assign(select,str,FC)){
					P.unassign(select,FC);
					continue;
				}
				node_expand++;
				bool result = recursiveBackTracking(P, FC, MRV, DEGREE, show_all);
				if(result && !show_all) return result;
				else if (result && show_all) {
					answer_cnt++;
				}
				P.unassign(select,FC);

			}
		}
		
		return false;
	}
}
void backTracking (Puzzle& P,bool FC = false, bool MRV = false, bool DEGREE = false, bool show_all = false){
	if(recursiveBackTracking(P, FC, MRV, DEGREE, show_all)){
		printf("Answer Found\n");
		for ( auto it = assignments.begin(); it != assignments.end(); ++it ){
    		cout <<"Word x: " << it->first.startX<<", y: "<<it->first.startY<<", len: "\
				<<it->first.len<<", direction: "<<it->first.direction\
				<< ", is filled with : "<< it->second <<endl;
		}
		printf("node expand: %d\n", node_expand);

	}
	else if (show_all)  printf("answer count: %d\n", answer_cnt);
	else { printf("Answer Not Found\n");}
}

int main(){
	bool FC = false;  // Apply forward checking
	bool MRV = true;; // Apply Minimum Remaining Variable selection
	bool DEGREE = false;// Apply most neighbors Variable selection
	bool show_all = true;// Find an answer and stop or go through all possible answers
	ifstream fin2("English Words 3000.txt");
	ifstream fin("puzzle.txt");
	if (!fin||!fin2) {
		cout << "Error opening file. Shutting down..." << endl;
		return 0;
	}
	for(string str; getline( fin2, str ,'\n'); ){ // store all vocabulary in vector
		stringstream ss(str); // remove weird characters
		string str2;
		ss>>str2;
		vocabulary[str2.size()].push_back(str2);
	}
	
	for(string str; getline( fin, str ,'\n'); ){
		Puzzle P(str);			 								// initial puzzle with input string
		P.setDomain();											// set domain initially with corresponding length of vocabulary
		P.setConstraint();
		backTracking( P, FC, MRV, DEGREE, show_all);
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

