FROM silkeh/clang:18

RUN mkdir /app

WORKDIR /app

SHELL ["/bin/sh", "-c"]
CMD clang-format -i --style=file src/*.cpp include/**/*.h test/*.cpp apps/*.cpp
