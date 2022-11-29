main: 
	g++ -pthread main.cpp lib.cpp -o main
red-worker:
	g++ red-worker.cpp lib.cpp -o red_worker