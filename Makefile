producer: 
	g++ -pthread red-producer.cpp lib.cpp -o producer
worker:
	g++ red-worker.cpp lib.cpp -o worker
generate-input:
	g++ generate-input.cpp -o generate_input