; ModuleID = 'bpftrace'
source_filename = "bpftrace"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "bpf"

%"struct map_internal_repr_t" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.0" = type { ptr, ptr }
%runtime_error_t = type <{ i64, i64, i32 }>

@LICENSE = global [4 x i8] c"GPL\00", section "license", !dbg !0
@AT_ = dso_local global %"struct map_internal_repr_t" zeroinitializer, section ".maps", !dbg !7
@ringbuf = dso_local global %"struct map_internal_repr_t.0" zeroinitializer, section ".maps", !dbg !28
@__bt__max_cpu_id = dso_local externally_initialized constant i64 0, section ".rodata", !dbg !42
@__bt__event_loss_counter = dso_local externally_initialized global [1 x [1 x i64]] zeroinitializer, section ".data.event_loss_counter", !dbg !44
@__bt__num_cpus = dso_local externally_initialized constant i64 0, section ".rodata", !dbg !48

; Function Attrs: nounwind
declare i64 @llvm.bpf.pseudo(i64 %0, i64 %1) #0

; Function Attrs: nounwind
define i64 @kprobe_f_1(ptr %0) #0 section "s_kprobe_f_1" !dbg !54 {
entry:
  %"$a" = alloca i64, align 8
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"$a")
  store i64 0, ptr %"$a", align 8
  %runtime_error_t5 = alloca %runtime_error_t, align 8
  %val_2 = alloca i64, align 8
  %val_1 = alloca i64, align 8
  %i = alloca i32, align 4
  %"@_key1" = alloca i64, align 8
  %runtime_error_t = alloca %runtime_error_t, align 8
  %initial_value = alloca i64, align 8
  %lookup_elem_val = alloca i64, align 8
  %"@_key" = alloca i64, align 8
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"@_key")
  store i64 0, ptr %"@_key", align 8
  %lookup_elem = call ptr inttoptr (i64 1 to ptr)(ptr @AT_, ptr %"@_key")
  call void @llvm.lifetime.start.p0(i64 -1, ptr %lookup_elem_val)
  %map_lookup_cond = icmp ne ptr %lookup_elem, null
  br i1 %map_lookup_cond, label %lookup_success, label %lookup_failure

lookup_success:                                   ; preds = %entry
  %1 = load i64, ptr %lookup_elem, align 8
  %2 = add i64 %1, 1
  store i64 %2, ptr %lookup_elem, align 8
  br label %lookup_merge

lookup_failure:                                   ; preds = %entry
  call void @llvm.lifetime.start.p0(i64 -1, ptr %initial_value)
  store i64 1, ptr %initial_value, align 8
  %update_elem = call i64 inttoptr (i64 2 to ptr)(ptr @AT_, ptr %"@_key", ptr %initial_value, i64 0)
  %3 = trunc i64 %update_elem to i32
  %4 = icmp sge i32 %3, 0
  br i1 %4, label %helper_merge, label %helper_failure

lookup_merge:                                     ; preds = %helper_merge, %lookup_success
  call void @llvm.lifetime.end.p0(i64 -1, ptr %lookup_elem_val)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %"@_key")
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"@_key1")
  store i64 0, ptr %"@_key1", align 8
  call void @llvm.lifetime.start.p0(i64 -1, ptr %i)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %val_1)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %val_2)
  store i32 0, ptr %i, align 4
  store i64 0, ptr %val_1, align 8
  store i64 0, ptr %val_2, align 8
  br label %while_cond

helper_failure:                                   ; preds = %lookup_failure
  call void @llvm.lifetime.start.p0(i64 -1, ptr %runtime_error_t)
  %5 = getelementptr %runtime_error_t, ptr %runtime_error_t, i64 0, i32 0
  store i64 30006, ptr %5, align 8
  %6 = getelementptr %runtime_error_t, ptr %runtime_error_t, i64 0, i32 1
  store i64 0, ptr %6, align 8
  %7 = getelementptr %runtime_error_t, ptr %runtime_error_t, i64 0, i32 2
  store i32 %3, ptr %7, align 4
  %ringbuf_output = call i64 inttoptr (i64 130 to ptr)(ptr @ringbuf, ptr %runtime_error_t, i64 20, i64 0)
  %ringbuf_loss = icmp slt i64 %ringbuf_output, 0
  br i1 %ringbuf_loss, label %event_loss_counter, label %counter_merge

helper_merge:                                     ; preds = %counter_merge, %lookup_failure
  call void @llvm.lifetime.end.p0(i64 -1, ptr %initial_value)
  br label %lookup_merge

event_loss_counter:                               ; preds = %helper_failure
  %get_cpu_id = call i64 inttoptr (i64 8 to ptr)() #2
  %8 = load i64, ptr @__bt__max_cpu_id, align 8
  %cpu.id.bounded = and i64 %get_cpu_id, %8
  %9 = getelementptr [1 x [1 x i64]], ptr @__bt__event_loss_counter, i64 0, i64 %cpu.id.bounded, i64 0
  %10 = load i64, ptr %9, align 8
  %11 = add i64 %10, 1
  store i64 %11, ptr %9, align 8
  br label %counter_merge

counter_merge:                                    ; preds = %event_loss_counter, %helper_failure
  call void @llvm.lifetime.end.p0(i64 -1, ptr %runtime_error_t)
  br label %helper_merge

while_cond:                                       ; preds = %lookup_success2, %lookup_merge
  %12 = load i32, ptr @__bt__num_cpus, align 4
  %13 = load i32, ptr %i, align 4
  %num_cpu.cmp = icmp ult i32 %13, %12
  br i1 %num_cpu.cmp, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %14 = load i32, ptr %i, align 4
  %lookup_percpu_elem = call ptr inttoptr (i64 195 to ptr)(ptr @AT_, ptr %"@_key1", i32 %14)
  %map_lookup_cond4 = icmp ne ptr %lookup_percpu_elem, null
  br i1 %map_lookup_cond4, label %lookup_success2, label %lookup_failure3

while_end:                                        ; preds = %error_failure, %counter_merge8, %while_cond
  call void @llvm.lifetime.end.p0(i64 -1, ptr %i)
  %15 = load i64, ptr %val_1, align 8
  call void @llvm.lifetime.end.p0(i64 -1, ptr %val_1)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %val_2)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %"@_key1")
  store i64 %15, ptr %"$a", align 8
  ret i64 0

lookup_success2:                                  ; preds = %while_body
  %16 = load i64, ptr %val_1, align 8
  %17 = load i64, ptr %lookup_percpu_elem, align 8
  %18 = add i64 %17, %16
  store i64 %18, ptr %val_1, align 8
  %19 = load i32, ptr %i, align 4
  %20 = add i32 %19, 1
  store i32 %20, ptr %i, align 4
  br label %while_cond

lookup_failure3:                                  ; preds = %while_body
  %21 = load i32, ptr %i, align 4
  %error_lookup_cond = icmp eq i32 %21, 0
  br i1 %error_lookup_cond, label %error_success, label %error_failure

error_success:                                    ; preds = %lookup_failure3
  call void @llvm.lifetime.start.p0(i64 -1, ptr %runtime_error_t5)
  %22 = getelementptr %runtime_error_t, ptr %runtime_error_t5, i64 0, i32 0
  store i64 30006, ptr %22, align 8
  %23 = getelementptr %runtime_error_t, ptr %runtime_error_t5, i64 0, i32 1
  store i64 1, ptr %23, align 8
  %24 = getelementptr %runtime_error_t, ptr %runtime_error_t5, i64 0, i32 2
  store i32 0, ptr %24, align 4
  %ringbuf_output6 = call i64 inttoptr (i64 130 to ptr)(ptr @ringbuf, ptr %runtime_error_t5, i64 20, i64 0)
  %ringbuf_loss9 = icmp slt i64 %ringbuf_output6, 0
  br i1 %ringbuf_loss9, label %event_loss_counter7, label %counter_merge8

error_failure:                                    ; preds = %lookup_failure3
  %25 = load i32, ptr %i, align 4
  br label %while_end

event_loss_counter7:                              ; preds = %error_success
  %get_cpu_id10 = call i64 inttoptr (i64 8 to ptr)() #2
  %26 = load i64, ptr @__bt__max_cpu_id, align 8
  %cpu.id.bounded11 = and i64 %get_cpu_id10, %26
  %27 = getelementptr [1 x [1 x i64]], ptr @__bt__event_loss_counter, i64 0, i64 %cpu.id.bounded11, i64 0
  %28 = load i64, ptr %27, align 8
  %29 = add i64 %28, 1
  store i64 %29, ptr %27, align 8
  br label %counter_merge8

counter_merge8:                                   ; preds = %event_loss_counter7, %error_success
  call void @llvm.lifetime.end.p0(i64 -1, ptr %runtime_error_t5)
  br label %while_end
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg %0, ptr nocapture %1) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg %0, ptr nocapture %1) #1

attributes #0 = { nounwind }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { memory(none) }

!llvm.dbg.cu = !{!50}
!llvm.module.flags = !{!52, !53}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "LICENSE", linkageName: "global", scope: !2, file: !2, type: !3, isLocal: false, isDefinition: true)
!2 = !DIFile(filename: "bpftrace.bpf.o", directory: ".")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 32, elements: !5)
!4 = !DIBasicType(name: "int8", size: 8, encoding: DW_ATE_signed)
!5 = !{!6}
!6 = !DISubrange(count: 4, lowerBound: 0)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(name: "AT_", linkageName: "global", scope: !2, file: !2, type: !9, isLocal: false, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 256, elements: !10)
!10 = !{!11, !17, !22, !25}
!11 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !13, size: 64)
!13 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 192, elements: !15)
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !{!16}
!16 = !DISubrange(count: 6, lowerBound: 0)
!17 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !2, file: !2, baseType: !18, size: 64, offset: 64)
!18 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !19, size: 64)
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 32, elements: !20)
!20 = !{!21}
!21 = !DISubrange(count: 1, lowerBound: 0)
!22 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !2, file: !2, baseType: !23, size: 64, offset: 128)
!23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !24, size: 64)
!24 = !DIBasicType(name: "int32", size: 32, encoding: DW_ATE_signed)
!25 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !2, file: !2, baseType: !26, size: 64, offset: 192)
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !27, size: 64)
!27 = !DIBasicType(name: "int64", size: 64, encoding: DW_ATE_signed)
!28 = !DIGlobalVariableExpression(var: !29, expr: !DIExpression())
!29 = distinct !DIGlobalVariable(name: "ringbuf", linkageName: "global", scope: !2, file: !2, type: !30, isLocal: false, isDefinition: true)
!30 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 128, elements: !31)
!31 = !{!32, !37}
!32 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !33, size: 64)
!33 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !34, size: 64)
!34 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 864, elements: !35)
!35 = !{!36}
!36 = !DISubrange(count: 27, lowerBound: 0)
!37 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !2, file: !2, baseType: !38, size: 64, offset: 64)
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !39, size: 64)
!39 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 8388608, elements: !40)
!40 = !{!41}
!41 = !DISubrange(count: 262144, lowerBound: 0)
!42 = !DIGlobalVariableExpression(var: !43, expr: !DIExpression())
!43 = distinct !DIGlobalVariable(name: "__bt__max_cpu_id", linkageName: "global", scope: !2, file: !2, type: !27, isLocal: false, isDefinition: true)
!44 = !DIGlobalVariableExpression(var: !45, expr: !DIExpression())
!45 = distinct !DIGlobalVariable(name: "__bt__event_loss_counter", linkageName: "global", scope: !2, file: !2, type: !46, isLocal: false, isDefinition: true)
!46 = !DICompositeType(tag: DW_TAG_array_type, baseType: !47, size: 64, elements: !20)
!47 = !DICompositeType(tag: DW_TAG_array_type, baseType: !27, size: 64, elements: !20)
!48 = !DIGlobalVariableExpression(var: !49, expr: !DIExpression())
!49 = distinct !DIGlobalVariable(name: "__bt__num_cpus", linkageName: "global", scope: !2, file: !2, type: !27, isLocal: false, isDefinition: true)
!50 = distinct !DICompileUnit(language: DW_LANG_C, file: !2, producer: "bpftrace", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly, globals: !51)
!51 = !{!0, !7, !28, !42, !44, !48}
!52 = !{i32 2, !"Debug Info Version", i32 3}
!53 = !{i32 7, !"uwtable", i32 0}
!54 = distinct !DISubprogram(name: "kprobe_f_1", linkageName: "kprobe_f_1", scope: !2, file: !2, type: !55, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !50, retainedNodes: !58)
!55 = !DISubroutineType(types: !56)
!56 = !{!27, !57}
!57 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!58 = !{!59}
!59 = !DILocalVariable(name: "ctx", arg: 1, scope: !54, file: !2, type: !57)
