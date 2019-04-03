all:
	g++ main.cpp -o main --std=c++11 -Wno-return-type
clean:
	rm main
