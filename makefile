all: server_fork.c server_fork.c
	gcc server_fork.c -o fork
	gcc server_select.c -o select
