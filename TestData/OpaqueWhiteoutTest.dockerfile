FROM alpine:latest

RUN mkdir -p /opaque/replaced/sub /opaque/kept && \
    touch /opaque/replaced/old.txt /opaque/replaced/sub/old-nested.txt /opaque/kept/kept.txt

# BuildKit's differ never emits opaque whiteouts itself: replacing a directory produces one whiteout per child instead
# (see the last step of DeleteTest). A file named like the marker is written to the layer tar verbatim, so creating
# one produces a genuine opaque whiteout. The opaque marker only hides lower layers, so the files added alongside it
# must survive, including "+before.txt", which sorts before the marker in the tar.
RUN touch /opaque/replaced/.wh..wh..opq /opaque/replaced/+before.txt /opaque/replaced/after.txt
