#pragma once

////////////////////////////////
//~ bgz: state ops
//
// The one way app state is changed. Code that wants a change pushes an op
// onto a queue, and _FR_UpdateAppState (frame.c) runs the queue: the only
// place an App_State is written. Everything else is handed a const App_State.
//
// The editor project's state ops (src\editor\state_ops there), without its
// metaprogram. An op is written by hand, in the layer it belongs to:
//
//    1. its params struct, <Name>OpParams - or none
//    2. the function, <Name>(App_State*, StateOp_List*, <Name>OpParams*)
//    3. STATE_OP(<Name>) under it, which makes StateOp_<Name>(ops): what
//       pushes it, and returns its params to fill in
//
//    typedef struct App_GoToSlideOpParams App_GoToSlideOpParams;
//    struct App_GoToSlideOpParams {
//       U64 slide_idx;
//    };
//
//    void
//    App_GoToSlide(App_State* app_state, StateOp_List* ops, App_GoToSlideOpParams* params) {
//       app_state->slide_idx = params->slide_idx;
//    }
//    STATE_OP(App_GoToSlide)
//
//    StateOp_App_GoToSlide(ops)->slide_idx = 3;
//
// An op another layer pushes has its params struct and its pusher,
// App_GoToSlideOpParams* StateOp_App_GoToSlide(StateOp_List* ops), declared in
// its layer's header. Code that is not an op pushes it and never calls it. An
// op that is running may call another op's function when it needs that done
// before it carries on: a push would only run after it returns.

// A layer's own functions and variables, which no other layer uses, are
// PRIVATE and start with _: PRIVATE String8 _Deck_Trim(...). What other
// layers use has no _ and is declared in the layer's header. Here, as in the
// editor project, ahead of every layer that uses it.
#define PRIVATE static

typedef struct App_State    App_State;
typedef struct StateOp_List StateOp_List;

typedef void StateOp_Proc(App_State* app_state, StateOp_List* ops, void* raw_params);

typedef struct StateOp StateOp;
struct StateOp {
   StateOp*      next;
   StateOp_Proc* proc;
   char*         name;   // the op's function's, to read in a debugger
   void*         params; // on the list's arena, zeroed when pushed. 0: the op takes none
};

// The queue. There is one, made in _EntryPoint (main.c): App_State's op_list.
// Its arena holds the ops and their params until they have run.
struct StateOp_List {
   Arena*   arena;
   StateOp* first;
   StateOp* last;
   U64      count;
};

void StateOp_Init(StateOp_List* ops);

// What STATE_OP's pushers call. Returns the op's params, 0 when params_size
// is 0.
void* StateOp_PushRaw(StateOp_List* ops, StateOp_Proc* proc, char* name, U64 params_size);

// Runs the ops in the order they were pushed, those pushed by an op as it
// runs too, and empties the queue. Called by _FR_UpdateAppState only.
void StateOp_ProcessList(App_State* app_state, StateOp_List* ops);

// Under an op's function: its pusher, StateOp_<Name>(ops). The function
// between is what the queue calls, with the params as it keeps them.
#define STATE_OP(Name)                                                                              \
   PRIVATE void _StateOp_##Name##_Run(App_State* app_state, StateOp_List* ops, void* raw_params) {  \
      Name(app_state, ops, (Name##OpParams*)raw_params);                                            \
   }                                                                                                \
   Name##OpParams* StateOp_##Name(StateOp_List* ops) {                                              \
      return (Name##OpParams*)StateOp_PushRaw(ops, _StateOp_##Name##_Run, #Name, sizeof(Name##OpParams)); \
   }

// The same for an op with no params, <Name>(App_State*, StateOp_List*).
#define STATE_OP_NO_PARAMS(Name)                                                                    \
   PRIVATE void _StateOp_##Name##_Run(App_State* app_state, StateOp_List* ops, void* raw_params) {  \
      UnusedVariable(raw_params);                                                                   \
      Name(app_state, ops);                                                                         \
   }                                                                                                \
   void StateOp_##Name(StateOp_List* ops) {                                                         \
      StateOp_PushRaw(ops, _StateOp_##Name##_Run, #Name, 0);                                        \
   }
