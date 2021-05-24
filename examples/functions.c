#include <stdio.h>

int times2(int x) {
    return x * 2;
}

int doSomething(int x) {
    return (x - 20) % 3;
}

int main() {
    int a = 20;
    int b = times2(a) * 50;
    int c = doSomething(b);

    printf("%d\n", c);
    return 0;
}
