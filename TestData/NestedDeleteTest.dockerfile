FROM alpine:3.22@sha256:49340b346798875b5592e5d420241f2f9cfba5f47e62ebb1446a1b34734e898f

RUN mkdir -p /nested/dir && touch /nested/dir/deleted-first.txt /nested/dir/deleted-with-dir.txt

RUN rm /nested/dir/deleted-first.txt

# Deleting the directory in the very next layer must not report "deleted-first.txt" as removed a second time, since it
# was already gone. Any layer in between would drop it from the tree and hide the problem.
RUN rm -rf /nested/dir
