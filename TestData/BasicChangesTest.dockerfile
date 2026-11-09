FROM alpine:3.22@sha256:49340b346798875b5592e5d420241f2f9cfba5f47e62ebb1446a1b34734e898f

RUN mkdir -p /app/config && printf 'one\n' > /app/config/settings.conf && printf 'hello\n' > /app/hello.txt

RUN printf 'two\n' >> /app/config/settings.conf

RUN printf 'new\n' > /app/config/extra.conf

# Changing only the mode still puts the file in the layer, so it shows as modified with an unchanged size.
RUN chmod 600 /app/hello.txt
