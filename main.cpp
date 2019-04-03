#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <unordered_map>

using namespace std;

class Word{
	public:
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
int node_expand =0;

class Puzzle{
	public:
		vector<Word> words;//Variables
		vector<Constraint> constraints; //constraints
		unordered_map<Word, vector<string> > domains;
		unordered_map<Word, vector<Constraint> > neighbors;
		

		Puzzle(){};
		Puzzle(string str){
			stringstream ss(str);
			int startX, startY, len;
			char direction;
			while(ss>>startX>>startY>>len>>direction){
				Word word(startX,startY,len,direction);
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
								this->constraints.push_back(c);
								this->neighbors[this->words[i]].push_back(c);
						}
					}
					else if(this->words[i].direction =='D'){
						if((this->words[i].startX >= this->words[j].startX && this->words[i].startX <= this->words[j].startX +this->words[j].len - 1)// x accepted
							&& (this->words[j].startY >= this->words[i].startY && this->words[j].startY <= this->words[i].startY + this->words[i].len - 1)){// y accepted
								int index_from = this->words[j].startY - this->words[i].startY;
								int index_to = this->words[i].startX - this->words[j].startX;
								Constraint c(this->words[i],this->words[j],index_from,index_to);
								this->constraints.push_back(c);
								this->neighbors[this->words[i]].push_back(c);
							}
					}
				}
			}
			
		}
		void assign(Word word, string str){
			assignments[word] = str;
			//cout<<"Assign " <<str<< " Successfully"<<endl;
		}

		void unassign(Word word){
			auto got = assignments.find(word);
			if(got == assignments.end()) return;
			assignments.erase(word);
			//printf("Unassign Successfully\n");

		}
		Word selectUnsignedWord(){
			for(int i = 0; i <this->words.size(); i++){
				auto got = assignments.find(this->words[i]);
				if(got == assignments.end()){
					return this->words[i];
				}
			}
		}
		bool nonConflict(Word select, string str){
			for(Constraint& c :this->neighbors[select]){
				auto got = assignments.find(c.to);
				if(got == assignments.end()) continue;// this neighbor doesn't have value yet
				if(str[c.from_index] != assignments[c.to][c.to_index])// different char in the same position
					return false;
			}
			return true;
		}


};
bool recursiveBackTracking(Puzzle P){
	if (assignments.size() == P.words.size()) return true;
	else{
		node_expand++;
		Word select = P.selectUnsignedWord();
		for(string str: P.domains[select]){
			if(P.nonConflict(select, str)) {
				P.assign(select,str);
				bool result = recursiveBackTracking(P);
				if(result) return result;
				P.unassign(select);
			}
		}
		return false;
	}
}
void backTracking (Puzzle P){
	if(recursiveBackTracking(P)){
		printf("Answer Found\n");
		for ( auto it = assignments.begin(); it != assignments.end(); ++it ){
    		cout <<"Word x: " << it->first.startX<<", y: "<<it->first.startY<<", len: "\
				<<it->first.len<<", direction: "<<it->first.direction\
				<< ", is filled with : "<< it->second <<endl;
		}
	}
	else{ printf("Answer Not Found\n");}
}

int main(){
	ifstream fin2("English Words 3000.txt");
	for(string str; getline( fin2, str ,'\n'); ){// store all vocabulary in vector
		stringstream ss(str); // remove weird characters
		string str2;
		ss>>str2;
		vocabulary[str2.size()].push_back(str2);
	}
	ifstream fin("puzzle.txt");
	if (!fin) {
		cout << "Error opening file. Shutting down..." << endl;
		return 0;
	}
	for(string str; getline( fin, str ,'\n'); ){
		Puzzle P(str);			 								// initial puzzle with input string
		P.setDomain();											// set domain initially with corresponding length of vocabulary
		P.setConstraint();
		backTracking(P);
		printf("node expand: %d\n", node_expand);
		node_expand =0;
		//ac3(P);
		str = "";
		assignments = {};
	}
	fin.close();
	fin2.close();
	return 0;
}

