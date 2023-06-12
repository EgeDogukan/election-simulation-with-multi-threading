


Functions:

	display_queue(int p):
		Our queues are types of C++ queues. We store our queues in pointer arrays. 				This function prints the queue in the given index.

	display_queue_pr(int p):
		Same implementation to print our priority queues.
	
	least_busy_station():
		Finds the least busy poll to send the newly arrived voter.

	least_bust_station_pr():
		Finds the least busy poll station for in pr queues.

	next_voter(void* polling station):
		This function is threaded for each poll. It sends the correct next voter to the poll.
		This is done by signaling the correct voter in the queue, and waiting a signal  				from that voter to signal the next voter.
		We also determine if it the poll is going to work(maintenance) in this function.

	voter(void* v_info):
		This is where the voters actually vote, it is also threaded for each of their own 				polls. We also pop from the relevant queues here.It waits until next_voter thread 				signals it and after it votes, it signals back to polling station.

	voter_arrives(void* arg):
		This function is where the new voters arrives to the election grounds.Then they 				are sent to the suitable queues with least density. This is also where they are 				pushed into said queues.

	logging_thread(void* arg):
		This is a separate running thread where we print logs into terminal.

	main(int argc, char** argv):
		We take required arguments here, initialize needed threads, mutexes. Here we 				also create our log file.


How to run:	

	Enter the terminal following line: “g++ main.cpp -o main” then 
	“./main -t <time parameter> -p <priority probability> -f <fail prob.> -c <poll. Station 			count> -n <terminal output time>”

How we solved starvation between queues:

	We take an account of how many consecutive ordinary people voted. Even if there are 			more than 5 ordinary people waiting in the line, if this consecutive value is higher than 			our predetermined max consecutive ordinary votes value, then the next voter is chosen 			from our pr queue.

General Info:
	Voters informations are kept in a struct with each voter has unique_id , polling_station which they are going to cast their vote and unique pthread_cond to wait until they are signaled from the polling station.
	Each polling station has its own voting_mutex and pthread_cond so that they can work simultaneously.
	Each queue has its own mutex, so we don’t access the queue (pop, insert) at the same time. Same can be said for the count of the candidates.
	The waiting_time (t in the project description) is set to 1 for testing purposes.



	
	
