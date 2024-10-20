#ifndef LUAAPI_H
#define LUAAPI_H

#ifndef LAPI_GDEXTENSION
#include "core/core_bind.h"
#include "core/object/ref_counted.h"
#else
#include <godot_cpp/classes/ref.hpp>
#endif

#include "core/io/dir_access.h"


#ifdef LAPI_GDEXTENSION
using namespace godot;
#endif

class LuaCoroutine;
class LuaObjectMetatable;
class LuaError;
class LuaAPI : public Object {
	GDCLASS(LuaAPI, Object);

protected:
	static void _bind_methods();

	static LuaAPI* singleton;
public:
	LuaAPI();
	~LuaAPI();

	static LuaAPI* get_singleton();

	void setHook(Callable hook, int mask, int count);
	void add_search_path(Ref<DirAccess> path)
	{
		searchPath.push_back(path);
	}
	TypedArray<Ref<DirAccess>> get_search_paths()
	{
		return searchPath;
	}
	void set_jit(bool _is_jit)
	{
		this->is_jit = _is_jit;
	}

	void setUseCallables(bool value);
	bool getUseCallables() const;

	void setObjectMetatable(Ref<LuaObjectMetatable> value);
	Ref<LuaObjectMetatable> getObjectMetatable() const;

	void setMemoryLimit(uint64_t limit);
	uint64_t getMemoryLimit() const;

	int configureGC(int what, int data);
	uint64_t getMemoryUsage() const;

	bool luaFunctionExists(String functionName);

	Variant pullVariant(String name);
	Variant callFunction(String functionName, Array args);
	// 获取全局表中的函数
	Ref<LuaError> getTableFuncMap(String table_name, HashMap<StringName, Callable>& func_map,HashMap<StringName, Variant> & member_map);
	void getLuaClassTable(String table_name,Object* _this);

	Variant getRegistryValue(String name);

	Ref<LuaError> setRegistryValue(String name, Variant var);
	Ref<LuaError> bindLibraries(TypedArray<String> libs);
	Ref<LuaError> doFile(String fileName);
	Ref<LuaError> doString(String code);
	Ref<LuaError> pushGlobalVariant(String name, Variant var);

	Ref<LuaCoroutine> newCoroutine();
	Ref<LuaCoroutine> getRunningCoroutine();

	struct lua_State *newThreadState();
	struct lua_State *getState();

	// enum HookMask {
	// 	HOOK_MASK_CALL = LUA_MASKCALL,
	// 	HOOK_MASK_RETURN = LUA_MASKRET,
	// 	HOOK_MASK_LINE = LUA_MASKLINE,
	// 	HOOK_MASK_COUNT = LUA_MASKCOUNT,
	// };

	// enum GCOption {
	// 	GC_STOP = LUA_GCSTOP,
	// 	GC_RESTART = LUA_GCRESTART,
	// 	GC_COLLECT = LUA_GCCOLLECT,
	// 	GC_COUNT = LUA_GCCOUNT,
	// 	GC_COUNTB = LUA_GCCOUNTB,
	// 	GC_STEP = LUA_GCSTEP,
	// 	GC_SETPAUSE = LUA_GCSETPAUSE,
	// 	GC_SETSTEPMUL = LUA_GCSETSTEPMUL,
	// };
	void chack_load()
	{
		if(lState == nullptr)
		{
			create_lua_state();
		}
	}
	bool is_load()
	{
		return lState != nullptr;
	}
	int get_version()
	{
		return version;
	}
	void reload_lua_state()
	{
		close_lua_state();
		create_lua_state();
	}
	void close_lua_state();

private:
	void create_lua_state();
private:
	bool useCallables = true;
	String startLuaFile = "res://init.lua";
	TypedArray<Ref<DirAccess>> searchPath;

	bool is_jit = false;
	String lua_start_string = "";
	class LuaState* state = nullptr;
	struct lua_State *lState = nullptr;
	int version = 0;

	Ref<LuaObjectMetatable> objectMetatable;

	static void *luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize);

	struct LuaAllocData {
		uint64_t memoryUsed = 0;
		uint64_t memoryLimit = 0;
	};

	LuaAllocData luaAllocData;

	Ref<LuaError> execute(int handlerIndex);
};

// 获取类的映射表
class LuaClassTable : public ScriptInstance {
public:
	void init(LuaAPI *lua,StringName name,Object* _self);
	bool has(const StringName &p_name) const
	{
		Object* self_ptr = self.get_ref();
		if(!is_valid() || self_ptr == nullptr)
		{
			return false;
		}
		return funcMap.has(p_name);
	}
	bool is_valid()const;
	virtual bool set(const StringName &p_name, const Variant &p_property) 
	{
		Object* self_ptr = self.get_ref();
		if(!is_valid() || self_ptr == nullptr)
		{
			return false;
		}
		if(propertyMap.has(p_name))
		{
			propertyMap[p_name] = p_property;
			return true;
		}
		return false;
	};
	virtual bool get(const StringName &p_name, Variant &r_property) const 
	{
		Object* self_ptr = self.get_ref();
		if(!is_valid() || self_ptr == nullptr)
		{
			return false;
		}
		if(propertyMap.has(p_name))
		{
			r_property = propertyMap[p_name];
			return true;
		}
		return false;
	}
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const 
	{
		return Variant::NIL;
	}
	virtual void get_property_list(List<PropertyInfo> *p_properties) const 
	{
		
	}
	virtual void get_property_state(List<Pair<StringName, Variant>> &state){
		
	}
	virtual void validate_property(PropertyInfo &p_property) const 
	{

	}
	virtual bool property_can_revert(const StringName &p_name) const 
	{
		return false;
	}
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const 
	{
		return false;
	}

	virtual void get_method_list(List<MethodInfo> *p_list) const 
	{

	}
	virtual bool has_method(const StringName &p_method) const 
	{
		return funcMap.has(p_method);
	}

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
	virtual Variant call_const(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
	virtual Ref<Script> get_script() const 
	{
		return Ref<Script>();
	}
	virtual ScriptLanguage *get_language() 
	{
		return nullptr;
	}
	
	virtual void notification(int p_notification, bool p_reversed = false) 
	{

	}
private:
	ObjectID objectID;
	WeakRef self;
	StringName table_name;
	mutable HashMap<StringName, Callable> funcMap;
	mutable HashMap<StringName, Variant> propertyMap;
	mutable bool is_init = false;
	mutable int version = 0;
};


// VARIANT_ENUM_CAST(LuaAPI::HookMask)
// VARIANT_ENUM_CAST(LuaAPI::GCOption)


#endif
