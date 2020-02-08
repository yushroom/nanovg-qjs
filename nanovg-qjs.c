#include "nanovg-qjs.h"
#include <nanovg.h>
#include <assert.h>
#include <cutils.h>
#include <quickjs.h>
#include <quickjs-libc.h>
#include <stdlib.h>
#include <string.h>

static int JS_ToFloat32(JSContext *ctx, float *pres, JSValueConst val)
{
	double f;
	int ret = JS_ToFloat64(ctx, &f, val);
	if (ret == 0)
		*pres = (float)f;
	return ret;
}


static NVGcontext *g_NVGcontext = NULL;

static JSClassID js_nanovg_paint_class_id;

static void js_nanovg_paint_finalizer(JSRuntime *rt, JSValue val)
{
	NVGpaint *p = JS_GetOpaque(val, js_nanovg_paint_class_id);
	if (p) {
		js_free_rt(rt, p);
	}
}

static JSValue js_nanovg_paint_ctor(JSContext *ctx,
									JSValueConst new_target,
									int argc, JSValueConst *argv)
{
	NVGpaint *p;
	JSValue obj = JS_UNDEFINED;
	JSValue proto;
	
	p = js_mallocz(ctx, sizeof(*p));
	if (!p)
		return JS_EXCEPTION;
	
	proto = JS_GetPropertyStr(ctx, new_target, "prototype");
	if (JS_IsException(proto))
		goto fail;
	obj = JS_NewObjectProtoClass(ctx, proto, js_nanovg_paint_class_id);
	JS_FreeValue(ctx, proto);
	if (JS_IsException(obj))
		goto fail;
	JS_SetOpaque(obj, p);
	return obj;
fail:
	js_free(ctx, p);
	JS_FreeValue(ctx, obj);
	return JS_EXCEPTION;
}

static JSClassDef js_nanovg_paint_class = {
	"Paint",
	.finalizer = js_nanovg_paint_finalizer,
};


static JSValue js_nanovg_wrap(JSContext *ctx, void *s, JSClassID classID)
{
	JSValue obj = JS_NewObjectClass(ctx, classID);
	if (JS_IsException(obj))
		return obj;
	JS_SetOpaque(obj, s);
	return obj;
}

int GetFloat32PropertyStr(JSContext *ctx, JSValueConst this_obj, const char *prop, float *pres)
{
	JSValue p = JS_GetPropertyStr(ctx, this_obj, prop);
	int ret = JS_ToFloat32(ctx, pres, p);
	JS_FreeValue(ctx, p);
	return ret;
}

int js_get_NVGcolor(JSContext *ctx, JSValueConst this_obj, NVGcolor *color)
{
	int ret = 0;
	ret = ret || GetFloat32PropertyStr(ctx, this_obj, "r", &color->r);
	ret = ret || GetFloat32PropertyStr(ctx, this_obj, "g", &color->g);
	ret = ret || GetFloat32PropertyStr(ctx, this_obj, "b", &color->b);
	ret = ret || GetFloat32PropertyStr(ctx, this_obj, "a", &color->a);
	return ret;
}


#define FUNC(fn) \
static JSValue js_nanovg_##fn(JSContext *ctx, JSValueConst this_value, int argc, JSValueConst *argv)

FUNC(Save)
{
	nvgSave(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(Restore)
{
	nvgReset(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(Rect)
{
	if (argc != 4)
		return JS_EXCEPTION;
	float x, y, w, h;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]) ||
		JS_ToFloat32(ctx, &w, argv[2]) ||
		JS_ToFloat32(ctx, &h, argv[3]))
		return JS_EXCEPTION;
	nvgRect(g_NVGcontext, x, y, w, h);
	return JS_UNDEFINED;
}

FUNC(Circle)
{
	if (argc != 3)
		return JS_EXCEPTION;
	float cx, cy, r;
	if (JS_ToFloat32(ctx, &cx, argv[0]) ||
		JS_ToFloat32(ctx, &cy, argv[1]) ||
		JS_ToFloat32(ctx, &r, argv[2]))
		return JS_EXCEPTION;
	nvgCircle(g_NVGcontext, cx, cy, r);
	return JS_UNDEFINED;
}

FUNC(PathWinding)
{
	if (argc != 1)
		return JS_EXCEPTION;
	int dir;
	if (JS_ToInt32(ctx, &dir, argv[0]))
		return JS_EXCEPTION;
	nvgPathWinding(g_NVGcontext, dir);
	return JS_UNDEFINED;
}

FUNC(MoveTo)
{
	if (argc != 2)
		return JS_EXCEPTION;
	float x, y;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]))
		return JS_EXCEPTION;
	nvgMoveTo(g_NVGcontext, x, y);
	return JS_UNDEFINED;
}

FUNC(LineTo)
{
	if (argc != 2)
		return JS_EXCEPTION;
	float x, y;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]))
		return JS_EXCEPTION;
	nvgLineTo(g_NVGcontext, x, y);
	return JS_UNDEFINED;
}

FUNC(FontBlur)
{
	if (argc != 1)
		return JS_EXCEPTION;
	float blur;
	if (JS_ToFloat32(ctx, &blur, argv[0]))
		return JS_EXCEPTION;
	nvgFontBlur(g_NVGcontext, blur);
	return JS_UNDEFINED;
}

FUNC(BeginPath)
{
	nvgBeginPath(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(RoundedRect)
{
	float x, y, w, h, r;
	if (argc != 5)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]) ||
		JS_ToFloat32(ctx, &w, argv[2]) ||
		JS_ToFloat32(ctx, &h, argv[3]) ||
		JS_ToFloat32(ctx, &r, argv[4]))
		return JS_EXCEPTION;
	nvgRoundedRect(g_NVGcontext, x, y, w, h, r);
	return JS_UNDEFINED;
}

FUNC(FillPaint)
{
	if (argc != 1)
		return JS_EXCEPTION;
	NVGpaint *paint = JS_GetOpaque(argv[0], js_nanovg_paint_class_id);
	if (!paint)
		return JS_EXCEPTION;
	nvgFillPaint(g_NVGcontext, *paint);
	return JS_UNDEFINED;
}

FUNC(Fill)
{
	nvgFill(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(StrokeColor)
{
	if (argc != 1)
		return JS_EXCEPTION;
	NVGcolor color;
	if (js_get_NVGcolor(ctx, argv[0], &color))
		return JS_EXCEPTION;
	nvgStrokeColor(g_NVGcontext, color);
	return JS_UNDEFINED;
}

FUNC(Stroke)
{
	nvgStroke(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(FillColor)
{
	if (argc != 1)
		return JS_EXCEPTION;
	NVGcolor color;
	if (js_get_NVGcolor(ctx, argv[0], &color))
		return JS_EXCEPTION;
	nvgFillColor(g_NVGcontext, color);
	return JS_UNDEFINED;
}

FUNC(LinearGradient)
{
	float sx, sy, ex, ey;
	NVGcolor icol, ocol;
	if (argc != 6)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &sx, argv[0]) ||
		JS_ToFloat32(ctx, &sy, argv[1]) ||
		JS_ToFloat32(ctx, &ex, argv[2]) ||
		JS_ToFloat32(ctx, &ey, argv[3]) ||
		js_get_NVGcolor(ctx, argv[4], &icol) ||
		js_get_NVGcolor(ctx, argv[5], &ocol))
		return JS_EXCEPTION;
	NVGpaint paint = nvgLinearGradient(g_NVGcontext, sx, sy, ex, ey, icol, ocol);
	NVGpaint *p = js_mallocz(ctx, sizeof(NVGpaint));
	*p = paint;
	return js_nanovg_wrap(ctx, p, js_nanovg_paint_class_id);
}

FUNC(BoxGradient)
{
	float x, y, w, h, r, f;
	NVGcolor icol, ocol;
	if (argc != 8)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]) ||
		JS_ToFloat32(ctx, &w, argv[2]) ||
		JS_ToFloat32(ctx, &h, argv[3]) ||
		JS_ToFloat32(ctx, &r, argv[4]) ||
		JS_ToFloat32(ctx, &f, argv[5]) ||
		js_get_NVGcolor(ctx, argv[6], &icol) ||
		js_get_NVGcolor(ctx, argv[7], &ocol))
		return JS_EXCEPTION;
	NVGpaint paint = nvgBoxGradient(g_NVGcontext, x, y, w, h, r, f, icol, ocol);
	NVGpaint *p = js_mallocz(ctx, sizeof(NVGpaint));
	*p = paint;
	return js_nanovg_wrap(ctx, p, js_nanovg_paint_class_id);
}

FUNC(RadialGradient)
{
	float cx, cy, inr, outr;
	NVGcolor icol, ocol;
	if (argc != 6)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &cx, argv[0]) ||
		JS_ToFloat32(ctx, &cy, argv[1]) ||
		JS_ToFloat32(ctx, &inr, argv[2]) ||
		JS_ToFloat32(ctx, &outr, argv[3]) ||
		js_get_NVGcolor(ctx, argv[4], &icol) ||
		js_get_NVGcolor(ctx, argv[5], &ocol))
		return JS_EXCEPTION;
	NVGpaint paint = nvgRadialGradient(g_NVGcontext, cx, cy, inr, outr, icol, ocol);
	NVGpaint *p = js_mallocz(ctx, sizeof(NVGpaint));
	*p = paint;
	return js_nanovg_wrap(ctx, p, js_nanovg_paint_class_id);
}

FUNC(TextBounds)
{
	float x, y;
	const char *str = NULL;
	if (argc != 3)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]))
		return JS_EXCEPTION;
	str = JS_ToCString(ctx, argv[2]);
	if (!str)
		return JS_EXCEPTION;
	float ret = nvgTextBounds(g_NVGcontext, x, y, str, NULL, NULL);
	return JS_NewFloat64(ctx, ret);
}

FUNC(TextBounds2)
{
	float x, y;
	const char *str = NULL;
	if (argc != 3)
		return JS_EXCEPTION;
	if (JS_ToFloat32(ctx, &x, argv[0]) ||
		JS_ToFloat32(ctx, &y, argv[1]))
	return JS_EXCEPTION;
	str = JS_ToCString(ctx, argv[2]);
	if (!str)
		return JS_EXCEPTION;
	float bounds[4] = {};
	float tw = nvgTextBounds(g_NVGcontext, x, y, str, NULL, bounds);
	JSValue e = JS_NewObject(ctx);
	JS_DefinePropertyValueStr(ctx, e, "width", JS_NewFloat64(ctx, tw), JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(ctx, e, "height", JS_NewFloat64(ctx, bounds[3]-bounds[1]), JS_PROP_C_W_E);
	return e;
}


FUNC(FontSize)
{
	if (argc != 1)
		return JS_EXCEPTION;
	double size;
	if (JS_ToFloat64(ctx, &size, argv[0]))
		return JS_EXCEPTION;
	nvgFontSize(g_NVGcontext, (float)size);
	return JS_UNDEFINED;
}

FUNC(FontFace)
{
	if (argc != 1)
		return JS_EXCEPTION;
	const char* str = JS_ToCString(ctx, argv[0]);
	if (!str)
		return JS_EXCEPTION;
	nvgFontFace(g_NVGcontext, str);
	JS_FreeCString(ctx, str);
	return JS_UNDEFINED;
}

FUNC(TextAlign)
{
	if (argc != 1)
		return JS_EXCEPTION;
	int align;
	if (JS_ToInt32(ctx, &align, argv[0]))
		return JS_EXCEPTION;
	nvgTextAlign(g_NVGcontext, align);
	return JS_UNDEFINED;
}

FUNC(Text)
{
	int x, y;
	const char *str = NULL;
	if (argc != 3)
		return JS_EXCEPTION;
	if (JS_ToInt32(ctx, &x, argv[0]))
		goto fail;
	if (JS_ToInt32(ctx, &y, argv[1]))
		goto fail;
	str = JS_ToCString(ctx, argv[2]);
	float ret = nvgText(g_NVGcontext, x, y, str, NULL);
	return JS_NewFloat64(ctx, ret);
fail:
	JS_FreeCString(ctx, str);
	return JS_EXCEPTION;
}

FUNC(SetNextFillHoverable)
{
	nvgSetNextFillHoverable(g_NVGcontext);
	return JS_UNDEFINED;
}

FUNC(IsFillHovered)
{
	int ret = nvgIsFillHovered(g_NVGcontext);
	return JS_NewBool(ctx, ret);
}

FUNC(IsNextFillClicked)
{
	int ret = nvgIsNextFillClicked(g_NVGcontext);
	return JS_NewBool(ctx, ret);
}


#define _JS_CFUNC_DEF(fn, length) JS_CFUNC_DEF(#fn, length, js_nanovg_##fn)
#define _JS_NANOVG_FLAG(name) JS_PROP_INT32_DEF(#name, NVG_##name, JS_PROP_CONFIGURABLE)

static const JSCFunctionListEntry js_nanovg_funcs[] = {
	_JS_CFUNC_DEF(Save, 0),
	_JS_CFUNC_DEF(Restore, 0),
	_JS_CFUNC_DEF(BeginPath, 0),
	_JS_CFUNC_DEF(RoundedRect, 5),
	_JS_CFUNC_DEF(FillPaint, 1),
	_JS_CFUNC_DEF(Fill, 0),
	_JS_CFUNC_DEF(StrokeColor, 4),
	_JS_CFUNC_DEF(Stroke, 0),
	_JS_CFUNC_DEF(FillColor, 4),
	_JS_CFUNC_DEF(LinearGradient, 6),
	_JS_CFUNC_DEF(BoxGradient, 8),
	_JS_CFUNC_DEF(RadialGradient, 6),
	_JS_CFUNC_DEF(TextBounds, 3),
	_JS_CFUNC_DEF(TextBounds2, 3),
	_JS_CFUNC_DEF(Rect, 4),
	_JS_CFUNC_DEF(Circle, 3),
	_JS_CFUNC_DEF(PathWinding, 1),
	_JS_CFUNC_DEF(MoveTo, 2),
	_JS_CFUNC_DEF(LineTo, 2),
	_JS_CFUNC_DEF(FontBlur, 1),
	_JS_CFUNC_DEF(FontSize, 1),
	_JS_CFUNC_DEF(FontFace, 1),
	_JS_CFUNC_DEF(TextAlign, 1),
	_JS_CFUNC_DEF(Text, 3),
	_JS_CFUNC_DEF(SetNextFillHoverable, 0),
	_JS_CFUNC_DEF(IsFillHovered, 0),
	_JS_CFUNC_DEF(IsNextFillClicked, 0),
	_JS_NANOVG_FLAG(ALIGN_LEFT),
	_JS_NANOVG_FLAG(ALIGN_CENTER),
	_JS_NANOVG_FLAG(ALIGN_RIGHT),
	_JS_NANOVG_FLAG(ALIGN_TOP),
	_JS_NANOVG_FLAG(ALIGN_MIDDLE),
	_JS_NANOVG_FLAG(ALIGN_BOTTOM),
	_JS_NANOVG_FLAG(HOLE),
};

static int js_nanovg_init(JSContext *ctx, JSModuleDef *m)
{
	JSValue paint_proto, paint_class;
	
	JS_NewClassID(&js_nanovg_paint_class_id);
	JS_NewClass(JS_GetRuntime(ctx), js_nanovg_paint_class_id, &js_nanovg_paint_class);
	
	paint_proto = JS_NewObject(ctx);
	JS_SetClassProto(ctx, js_nanovg_paint_class_id, paint_proto);
	paint_class = JS_NewCFunction2(ctx, js_nanovg_paint_ctor, "Paint", 0, JS_CFUNC_constructor, 0);
	JS_SetConstructor(ctx, paint_class, paint_proto);
	
	JS_SetModuleExport(ctx, m, "Paint", paint_class);
    JS_SetModuleExportList(ctx, m, js_nanovg_funcs, countof(js_nanovg_funcs));
	return 0;
}

JSModuleDef *js_init_module_nanovg(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_nanovg_init);
    if (!m)
        return NULL;
	JS_AddModuleExport(ctx, m, "Paint");
    JS_AddModuleExportList(ctx, m, js_nanovg_funcs, countof(js_nanovg_funcs));
    return m;
}

void js_nanovg_init_with_context(struct NVGcontext *vg)
{
	g_NVGcontext = vg;
}
