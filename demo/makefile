all:tcpepoll client

tcpepoll:tcpepoll.cpp
	g++ -g tcpepoll.cpp -o tcpepoll -pthread -std=c++11

client:client.cpp
	g++ -g client.cpp -o client -pthread -std=c++11

clean:
	rm -f tcpepoll client