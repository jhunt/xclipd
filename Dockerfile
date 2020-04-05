FROM alpine:3 AS build
RUN apk update && apk add make gcc libc-dev libx11-dev

COPY . .
RUN make \
 && mv xclipd /usr/bin

FROM alpine:3
RUN apk update && apk add libx11 && rm -f /var/cache/apk/*
COPY --from=build /usr/bin/xclipd /usr/bin/xclipd

ENTRYPOINT ["/usr/bin/xclipd"]
