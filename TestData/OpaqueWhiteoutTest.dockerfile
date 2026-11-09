FROM alpine:3.22@sha256:49340b346798875b5592e5d420241f2f9cfba5f47e62ebb1446a1b34734e898f

RUN mkdir -p /opaque/replaced/sub /opaque/kept /opaque/merged/sub && \
    touch /opaque/replaced/old.txt /opaque/replaced/sub/old-nested.txt /opaque/kept/kept.txt && \
    touch /opaque/merged/old.txt /opaque/merged/recreated.txt /opaque/merged/sub/old-nested.txt

# BuildKit's differ never emits opaque whiteouts itself: replacing a directory produces one whiteout per child instead
# (see the last step of DeleteTest). A file named like the marker is written to the layer tar verbatim, so creating
# one produces a genuine opaque whiteout. The opaque marker only hides lower layers, so the files added alongside it
# must survive, including "+before.txt", which sorts before the marker in the tar.
RUN touch /opaque/replaced/.wh..wh..opq /opaque/replaced/+before.txt /opaque/replaced/after.txt

# Entries that the opaque directory's layer also contains must not be merged with the lower layer: "sub" appears in
# both layers, but its lower contents are still hidden, so only "sub/new-nested.txt" should remain inside it.
RUN echo "new" > /opaque/merged/recreated.txt && \
    touch /opaque/merged/.wh..wh..opq /opaque/merged/sub/new-nested.txt
