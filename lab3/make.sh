#!/bin/bash

if gcc $1 -o result.out; then
    shift 1;
    ./result.out $@;
else
    echo "Ошибка компиляции";
fi
