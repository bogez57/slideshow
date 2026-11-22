////////////////////////////////
//~ bgz: state ops
//
// See state_ops.h.

void
StateOp_Init(StateOp_List* ops) {
   MemoryZeroStruct(ops);
   ops->arena = ArenaAlloc();
}

void*
StateOp_PushRaw(StateOp_List* ops, StateOp_Proc* proc, char* name, U64 params_size) {
   StateOp* op = PushArray(ops->arena, StateOp, 1);
   op->proc = proc;
   op->name = name;
   if (params_size > 0) {
      op->params = PushArray(ops->arena, U8, params_size);
   }

   SLLQueuePush(ops->first, ops->last, op);
   ops->count += 1;

   return op->params;
}

void
StateOp_ProcessList(App_State* app_state, StateOp_List* ops) {
   U64 processed_count = 0;

   // An op may push more as it runs. Those land at the end of the queue, so
   // this loop comes to them in the same pass.
   for (StateOp* op = ops->first; op != 0; op = op->next) {
      Assert(processed_count++ < 10000);
      op->proc(app_state, ops, op->params);
   }

   ops->first = 0;
   ops->last = 0;
   ops->count = 0;
   ArenaClear(ops->arena);
}
