FROM debian:bookworm-slim AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential libssl-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY Makefile ./
COPY include/ include/
COPY src/ src/
RUN make MODE=release all

FROM debian:bookworm-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends libssl3 ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --system --no-create-home bot
COPY --from=build /src/run /usr/local/bin/run
USER bot
ENTRYPOINT ["/usr/local/bin/run"]