FROM alpine:latest

RUN mkdir -p /path/to/ && touch /path/to/file.txt && \
    mkdir -p /path/to/directory/1 /path/to/directory/2 && touch /path/to/directory/1/1.txt /path/to/directory/2/2.txt

RUN rm -rf /path/to/file.txt && \
    rm -rf /path/to/directory

RUN mkdir -p /path/deleted/dirs/should/not/be/visible

RUN mkdir -p /big/tree && \
    for i in $(seq 1 1000); do \
      mkdir -p /big/tree/dir$i && \
      touch /big/tree/dir$i/file1.txt /big/tree/dir$i/file2.txt; \
    done

RUN rm -rf /big/tree && mkdir -p /big/tree