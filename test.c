int x = 1;
int result = 1;

while (x <= 5) {
    result = result * x;
    x = x + 1;
}

if (result == 120) {
    print(result);
} else {
    print(0);
}