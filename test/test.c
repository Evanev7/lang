struct a {
    struct b * b_pointer;
    int c;
};

typedef struct b {
    struct a * a_pointer;
    void * d;
} b;

int main(int argc, char** argv) {
        return 0;
}
