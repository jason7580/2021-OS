#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {}

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
	pthread_create(&t, 0, ConsumerController::process, (void*)this);
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
	ConsumerController* consumerController = (ConsumerController*)arg;
	while(true){
		usleep(consumerController->check_period);
		if(consumerController->worker_queue->get_size() > consumerController->high_threshold){
			Consumer* newconsumer = new Consumer(consumerController->worker_queue, consumerController->writer_queue, consumerController->transformer);
			consumerController->consumers.push_back(newconsumer);
			consumerController->consumers.back()->start();
			std::cout << "Scaling up consumers from " << consumerController->consumers.size()-1 << " to " << consumerController->consumers.size() << '\n';
		} else if((consumerController->worker_queue->get_size() < consumerController->low_threshold) && (consumerController->consumers.size() > 1)){
			consumerController->consumers.back()->cancel();
			consumerController->consumers.pop_back();
			std::cout << "Scaling down consumers from " << consumerController->consumers.size()+1 << " to " << consumerController->consumers.size() << '\n';
		}
	}
}

#endif // CONSUMER_CONTROLLER_HPP
