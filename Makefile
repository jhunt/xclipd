xclipd: src/xclipd.c
	$(CC) -Wall -Wextra -o $@ $< -lX11

docker:
	docker build -t docker/xclipd .

.PHONY: docker
