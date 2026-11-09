FROM alpine:3.22@sha256:49340b346798875b5592e5d420241f2f9cfba5f47e62ebb1446a1b34734e898f

RUN mkdir -p /links/lib/real /links/bin /links/nested/deeper && \
    echo "hello" > /links/lib/real/libfoo.so.1.2.3 && \
    echo "data" > /links/data.txt

# Symlinks: relative targets resolve against the link's own directory, absolute ones against the root.
RUN cd /links/lib/real && ln -s libfoo.so.1.2.3 libfoo.so.1 && \
    cd /links/lib && ln -s real/libfoo.so.1 libfoo.so && \
    cd /links/bin && ln -s ../lib/libfoo.so libfoo-from-bin && \
    cd /links/nested/deeper && ln -s ../../data.txt data-relative && \
    ln -s /links/data.txt /links/nested/deeper/data-absolute && \
    ln -s ../lib /links/bin/lib-dir && \
    ln -s does-not-exist /links/dangling

# Hardlinks: in the tar, the target is an archive-root path such as "links/hard/nested/hardlink.txt", not
# relative to the link. Which name becomes the link depends on the order the layer differ walks the files in.
# A layer can only hardlink to members of its own tar, so these must be created alongside their targets.
RUN mkdir -p /links/hard/nested && \
    echo "shared" > /links/hard/original.txt && \
    ln /links/hard/original.txt /links/hard/nested/hardlink.txt

# Hardlinking to a file from an earlier layer produces a full copy instead of a hardlink entry.
RUN ln /links/data.txt /links/nested/deeper/data-copied-not-linked
