FROM silkeh/clang:18

RUN mkdir /app

WORKDIR /app

CMD ["clang-format", "-i", "-style=file", "src/*.cpp", "include/**/*.h", "test/*.cpp", "apps/*.cpp"]
