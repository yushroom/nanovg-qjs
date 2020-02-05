#ifndef NANOVG_QJS_H
#define NANOVG_QJS_H
#include <quickjs.h>

struct NVGcontext;
void js_nanovg_init_with_context(struct NVGcontext *vg);

JSModuleDef *js_init_module_nanovg(JSContext *ctx, const char *module_name);


#endif /* NANOVG_QJS_H */
