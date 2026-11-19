#include <stdio.h>
#include <dlfcn.h>
typedef void (*encrypt_fn)(void);
int main(void) {
 void *h = dlopen("./libcrypto_demo.so", RTLD_NOW);
 if (!h) { puts(dlerror()); return 1; }
 dlerror();
 encrypt_fn f = (encrypt_fn)dlsym(h, "encrypt_message");
 const char *e = dlerror();
 if (e) { puts(e); dlclose(h); return 1; }
 f();
 dlclose(h);
 return 0;
}
