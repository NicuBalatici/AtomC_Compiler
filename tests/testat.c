struct S {
    int n;
    char text[16];
};

struct C {
    int m;
};

struct S a;
struct S v[10];
struct C z;

void f(char text[], int i, char ch) {
    int j;
    text[i] = ch;
    j = 0;
    return;
}

int h(int x, int y) {
    double p;
    p = 10;
    char j;
    j = 'a';
    double l;
    // l = p + j; // Both operands must be scalars and not structs; source must be convertible to destination
    // j = p + j; // error: result type is source type
    double k;

    if (x > 0 && x < y) {
        f(v[x].text, y, '#'); // Function call must have the same number of arguments as parameters
        // f(); // error: function call must have same number of arguments as parameters
        f("text", j, 'a'); // Argument types must be convertible to parameter types
        // f(1, j, '%'); // error: argument types not convertible
        // p = f; // error: a function can only be called, not assigned
        return 1;
    }

    // s = j + l; // error: identifier must exist in the symbol table
    struct S a;
    struct S b;
    struct S c;
    // c = a + b; // error: both operands must be scalars and not structs
    // c = a * b; // same type error
    // c.x; // error: struct field must exist
    // c.n; // would be ok
    if (1) {} // test for scalar condition

    // 10 = p; // error: destination must be a left-value
    // c.n = 1; // ok
    // x.n; // error: x is not a struct
    // int x[10]; // only arrays can be indexed
    // x['a']; // depends on interpretation, usually valid if x is array

    // Unary expression checks
    // x = -c; // error: unary minus requires scalar
    // x = !c; // error: logical NOT requires scalar
    // x = !x; // ok: result is int
    // x = -x; // ok
    // j = -x; // ok
    // j = !x; // ok
    // k = -k; // ok

    // p=f; // error: only functions can be called

    c.n = a.n + b.n; // struct field access is valid only on structs

    int v[100];
    char h[101];
    // v.x; // error: field access only valid on structs
    // v[100] = h[101]; // out-of-bounds, technically undefined behavior
    // v[2.14]; // error: array index must be convertible to int
    // v['a']; // usually OK: char is implicitly converted to int

    return 1;
    // return j; // error: return expression must be convertible to function return type
}

void g() {
    int x;
	// 1=x; // error: destination must be a left-value
    x = 1; // valid assignment, left-value and scalar
    struct S s;
    double y;
    y = 3.14;
	// const char *str = "hello";
	// 1=str;
    // x = y; // error: double to int implicit assignment usually not allowed in stricter settings
    y = x; // OK: int to double is allowed

    // IF condition must be scalar
    if (x) {}

    int z;
    // z = x || y; // result is int

    // char z;
    // z = x || y; // error: result is int, can't assign to char in strict typing

    // z = x && y;
    // z = x == y;
    // z = x < y;

    // Both operands must be scalars and not structs
    // z = a || v[0]; // error
    // z = a && v[0]; // error
    // z = a == v[0]; // error
    // z = a < v[0]; // error

    // IF condition non-scalar
    // if (s) {} // error: struct not allowed as condition

    // WHILE condition must be scalar
    while (x) {}

    // WHILE condition non-scalar
    // while (s) {} // error

    // RETURN in void function without expression
    return;

    // RETURN in void function with expression
    // return x; // error

    // 1 = x; // error: destination must be a left-value
}