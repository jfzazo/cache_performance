# cache_performance
This is a small example where the impact of the cache can be measure in a Linux environment


## Installation
The program can be compiled with the following command:

	gcc cache_test.c -o cache_test

## Execution

The invokation should be like:

	./cache_test.c OPERATION SIZE

Where OPERATION is one of the following options:
	- R. Read from a buffer
	- W. Write to a buffer
	- RW. Read from a buffer and following this operation perform a write
	- WR. Write to a buffer and following this operation perform a read

## Output 

The total spent time should appear on the standar output (precision of us)	
