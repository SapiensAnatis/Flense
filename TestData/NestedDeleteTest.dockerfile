FROM alpine:latest

RUN mkdir -p /nested/dir && touch /nested/dir/deleted-first.txt /nested/dir/deleted-with-dir.txt

RUN rm /nested/dir/deleted-first.txt

# Deleting the directory in the very next layer must not report "deleted-first.txt" as removed a second time, since it
# was already gone. Any layer in between would drop it from the tree and hide the problem.
RUN rm -rf /nested/dir
