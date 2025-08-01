; ModuleID = 'bpftrace'
source_filename = "bpftrace"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "bpf"

%"struct map_internal_repr_t" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.0" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.1" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.2" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.3" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.4" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.5" = type { ptr, ptr, ptr, ptr }
%"struct map_internal_repr_t.6" = type { ptr, ptr }
%ustack_key = type { i64, i64, i32, i32 }

@LICENSE = global [4 x i8] c"GPL\00", section "license", !dbg !0
@AT_x = dso_local global %"struct map_internal_repr_t" zeroinitializer, section ".maps", !dbg !7
@AT_y = dso_local global %"struct map_internal_repr_t.0" zeroinitializer, section ".maps", !dbg !26
@AT_z = dso_local global %"struct map_internal_repr_t.1" zeroinitializer, section ".maps", !dbg !28
@stack_perf_127 = dso_local global %"struct map_internal_repr_t.2" zeroinitializer, section ".maps", !dbg !30
@stack_bpftrace_6 = dso_local global %"struct map_internal_repr_t.3" zeroinitializer, section ".maps", !dbg !54
@stack_bpftrace_127 = dso_local global %"struct map_internal_repr_t.4" zeroinitializer, section ".maps", !dbg !63
@stack_scratch = dso_local global %"struct map_internal_repr_t.5" zeroinitializer, section ".maps", !dbg !65
@ringbuf = dso_local global %"struct map_internal_repr_t.6" zeroinitializer, section ".maps", !dbg !75
@__bt__event_loss_counter = dso_local externally_initialized global [1 x [1 x i64]] zeroinitializer, section ".data.event_loss_counter", !dbg !89
@__bt__max_cpu_id = dso_local externally_initialized constant i64 0, section ".rodata", !dbg !93

; Function Attrs: nounwind
declare i64 @llvm.bpf.pseudo(i64 %0, i64 %1) #0

; Function Attrs: nounwind
define i64 @kprobe_f_1(ptr %0) #0 section "s_kprobe_f_1" !dbg !99 {
entry:
  %"@z_key" = alloca i64, align 8
  %lookup_stack_scratch_key21 = alloca i32, align 4
  %stack_key18 = alloca %ustack_key, align 8
  %"@y_key" = alloca i64, align 8
  %lookup_stack_scratch_key5 = alloca i32, align 4
  %stack_key2 = alloca %ustack_key, align 8
  %"@x_key" = alloca i64, align 8
  %lookup_stack_scratch_key = alloca i32, align 4
  %stack_key = alloca %ustack_key, align 8
  call void @llvm.lifetime.start.p0(i64 -1, ptr %stack_key)
  call void @llvm.memset.p0.i64(ptr align 1 %stack_key, i8 0, i64 24, i1 false)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %lookup_stack_scratch_key)
  store i32 0, ptr %lookup_stack_scratch_key, align 4
  %lookup_stack_scratch_map = call ptr inttoptr (i64 1 to ptr)(ptr @stack_scratch, ptr %lookup_stack_scratch_key)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %lookup_stack_scratch_key)
  %lookup_stack_scratch_cond = icmp ne ptr %lookup_stack_scratch_map, null
  br i1 %lookup_stack_scratch_cond, label %lookup_stack_scratch_merge, label %lookup_stack_scratch_failure

stack_scratch_failure:                            ; preds = %lookup_stack_scratch_failure
  br label %merge_block

merge_block:                                      ; preds = %stack_scratch_failure, %get_stack_success, %get_stack_fail
  %1 = getelementptr %ustack_key, ptr %stack_key, i64 0, i32 2
  %get_pid_tgid = call i64 inttoptr (i64 14 to ptr)() #4
  %2 = lshr i64 %get_pid_tgid, 32
  %pid = trunc i64 %2 to i32
  store i32 %pid, ptr %1, align 4
  %3 = getelementptr %ustack_key, ptr %stack_key, i64 0, i32 3
  store i32 0, ptr %3, align 4
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"@x_key")
  store i64 0, ptr %"@x_key", align 8
  %update_elem1 = call i64 inttoptr (i64 2 to ptr)(ptr @AT_x, ptr %"@x_key", ptr %stack_key, i64 0)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %"@x_key")
  call void @llvm.lifetime.start.p0(i64 -1, ptr %stack_key2)
  call void @llvm.memset.p0.i64(ptr align 1 %stack_key2, i8 0, i64 24, i1 false)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %lookup_stack_scratch_key5)
  store i32 0, ptr %lookup_stack_scratch_key5, align 4
  %lookup_stack_scratch_map6 = call ptr inttoptr (i64 1 to ptr)(ptr @stack_scratch, ptr %lookup_stack_scratch_key5)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %lookup_stack_scratch_key5)
  %lookup_stack_scratch_cond9 = icmp ne ptr %lookup_stack_scratch_map6, null
  br i1 %lookup_stack_scratch_cond9, label %lookup_stack_scratch_merge8, label %lookup_stack_scratch_failure7

lookup_stack_scratch_failure:                     ; preds = %entry
  br label %stack_scratch_failure

lookup_stack_scratch_merge:                       ; preds = %entry
  %probe_read_kernel = call i64 inttoptr (i64 113 to ptr)(ptr %lookup_stack_scratch_map, i32 1016, ptr null)
  %get_stack = call i64 inttoptr (i64 67 to ptr)(ptr %0, ptr %lookup_stack_scratch_map, i32 1016, i64 256)
  %4 = icmp sge i64 %get_stack, 0
  br i1 %4, label %get_stack_success, label %get_stack_fail

get_stack_success:                                ; preds = %lookup_stack_scratch_merge
  %5 = udiv i64 %get_stack, 8
  %6 = getelementptr %ustack_key, ptr %stack_key, i64 0, i32 1
  store i64 %5, ptr %6, align 8
  %7 = trunc i64 %5 to i8
  %murmur_hash_2 = call i64 @murmur_hash_2(ptr %lookup_stack_scratch_map, i8 %7, i64 1)
  %8 = getelementptr %ustack_key, ptr %stack_key, i64 0, i32 0
  store i64 %murmur_hash_2, ptr %8, align 8
  %update_elem = call i64 inttoptr (i64 2 to ptr)(ptr @stack_bpftrace_127, ptr %stack_key, ptr %lookup_stack_scratch_map, i64 0)
  br label %merge_block

get_stack_fail:                                   ; preds = %lookup_stack_scratch_merge
  br label %merge_block

stack_scratch_failure3:                           ; preds = %lookup_stack_scratch_failure7
  br label %merge_block4

merge_block4:                                     ; preds = %stack_scratch_failure3, %get_stack_success10, %get_stack_fail11
  %9 = getelementptr %ustack_key, ptr %stack_key2, i64 0, i32 2
  %get_pid_tgid15 = call i64 inttoptr (i64 14 to ptr)() #4
  %10 = lshr i64 %get_pid_tgid15, 32
  %pid16 = trunc i64 %10 to i32
  store i32 %pid16, ptr %9, align 4
  %11 = getelementptr %ustack_key, ptr %stack_key2, i64 0, i32 3
  store i32 0, ptr %11, align 4
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"@y_key")
  store i64 0, ptr %"@y_key", align 8
  %update_elem17 = call i64 inttoptr (i64 2 to ptr)(ptr @AT_y, ptr %"@y_key", ptr %stack_key2, i64 0)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %"@y_key")
  call void @llvm.lifetime.start.p0(i64 -1, ptr %stack_key18)
  call void @llvm.memset.p0.i64(ptr align 1 %stack_key18, i8 0, i64 24, i1 false)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %lookup_stack_scratch_key21)
  store i32 0, ptr %lookup_stack_scratch_key21, align 4
  %lookup_stack_scratch_map22 = call ptr inttoptr (i64 1 to ptr)(ptr @stack_scratch, ptr %lookup_stack_scratch_key21)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %lookup_stack_scratch_key21)
  %lookup_stack_scratch_cond25 = icmp ne ptr %lookup_stack_scratch_map22, null
  br i1 %lookup_stack_scratch_cond25, label %lookup_stack_scratch_merge24, label %lookup_stack_scratch_failure23

lookup_stack_scratch_failure7:                    ; preds = %merge_block
  br label %stack_scratch_failure3

lookup_stack_scratch_merge8:                      ; preds = %merge_block
  call void @llvm.memset.p0.i64(ptr align 1 %lookup_stack_scratch_map6, i8 0, i64 48, i1 false)
  %get_stack12 = call i64 inttoptr (i64 67 to ptr)(ptr %0, ptr %lookup_stack_scratch_map6, i32 48, i64 256)
  %12 = icmp sge i64 %get_stack12, 0
  br i1 %12, label %get_stack_success10, label %get_stack_fail11

get_stack_success10:                              ; preds = %lookup_stack_scratch_merge8
  %13 = udiv i64 %get_stack12, 8
  %14 = getelementptr %ustack_key, ptr %stack_key2, i64 0, i32 1
  store i64 %13, ptr %14, align 8
  %15 = trunc i64 %13 to i8
  %murmur_hash_213 = call i64 @murmur_hash_2(ptr %lookup_stack_scratch_map6, i8 %15, i64 1)
  %16 = getelementptr %ustack_key, ptr %stack_key2, i64 0, i32 0
  store i64 %murmur_hash_213, ptr %16, align 8
  %update_elem14 = call i64 inttoptr (i64 2 to ptr)(ptr @stack_bpftrace_6, ptr %stack_key2, ptr %lookup_stack_scratch_map6, i64 0)
  br label %merge_block4

get_stack_fail11:                                 ; preds = %lookup_stack_scratch_merge8
  br label %merge_block4

stack_scratch_failure19:                          ; preds = %lookup_stack_scratch_failure23
  br label %merge_block20

merge_block20:                                    ; preds = %stack_scratch_failure19, %get_stack_success27, %get_stack_fail28
  %17 = getelementptr %ustack_key, ptr %stack_key18, i64 0, i32 2
  %get_pid_tgid32 = call i64 inttoptr (i64 14 to ptr)() #4
  %18 = lshr i64 %get_pid_tgid32, 32
  %pid33 = trunc i64 %18 to i32
  store i32 %pid33, ptr %17, align 4
  %19 = getelementptr %ustack_key, ptr %stack_key18, i64 0, i32 3
  store i32 0, ptr %19, align 4
  call void @llvm.lifetime.start.p0(i64 -1, ptr %"@z_key")
  store i64 0, ptr %"@z_key", align 8
  %update_elem34 = call i64 inttoptr (i64 2 to ptr)(ptr @AT_z, ptr %"@z_key", ptr %stack_key18, i64 0)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %"@z_key")
  ret i64 0

lookup_stack_scratch_failure23:                   ; preds = %merge_block4
  br label %stack_scratch_failure19

lookup_stack_scratch_merge24:                     ; preds = %merge_block4
  %probe_read_kernel26 = call i64 inttoptr (i64 113 to ptr)(ptr %lookup_stack_scratch_map22, i32 1016, ptr null)
  %get_stack29 = call i64 inttoptr (i64 67 to ptr)(ptr %0, ptr %lookup_stack_scratch_map22, i32 1016, i64 256)
  %20 = icmp sge i64 %get_stack29, 0
  br i1 %20, label %get_stack_success27, label %get_stack_fail28

get_stack_success27:                              ; preds = %lookup_stack_scratch_merge24
  %21 = udiv i64 %get_stack29, 8
  %22 = getelementptr %ustack_key, ptr %stack_key18, i64 0, i32 1
  store i64 %21, ptr %22, align 8
  %23 = trunc i64 %21 to i8
  %murmur_hash_230 = call i64 @murmur_hash_2(ptr %lookup_stack_scratch_map22, i8 %23, i64 1)
  %24 = getelementptr %ustack_key, ptr %stack_key18, i64 0, i32 0
  store i64 %murmur_hash_230, ptr %24, align 8
  %update_elem31 = call i64 inttoptr (i64 2 to ptr)(ptr @stack_perf_127, ptr %stack_key18, ptr %lookup_stack_scratch_map22, i64 0)
  br label %merge_block20

get_stack_fail28:                                 ; preds = %lookup_stack_scratch_merge24
  br label %merge_block20
}

; Function Attrs: alwaysinline nounwind
define internal i64 @murmur_hash_2(ptr %0, i8 %1, i64 %2) #1 section "helpers" {
entry:
  %k = alloca i64, align 8
  %i = alloca i8, align 1
  %id = alloca i64, align 8
  %seed_addr = alloca i64, align 8
  %nr_stack_frames_addr = alloca i8, align 1
  call void @llvm.lifetime.start.p0(i64 -1, ptr %nr_stack_frames_addr)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %seed_addr)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %id)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %i)
  call void @llvm.lifetime.start.p0(i64 -1, ptr %k)
  store i8 %1, ptr %nr_stack_frames_addr, align 1
  store i64 %2, ptr %seed_addr, align 8
  %3 = load i8, ptr %nr_stack_frames_addr, align 1
  %4 = zext i8 %3 to i64
  %5 = mul i64 %4, -4132994306676758123
  %6 = load i64, ptr %seed_addr, align 8
  %7 = xor i64 %6, %5
  store i64 %7, ptr %id, align 8
  store i8 0, ptr %i, align 1
  br label %while_cond

while_cond:                                       ; preds = %while_body, %entry
  %8 = load i8, ptr %nr_stack_frames_addr, align 1
  %9 = load i8, ptr %i, align 1
  %length.cmp = icmp ult i8 %9, %8
  br i1 %length.cmp, label %while_body, label %while_end

while_body:                                       ; preds = %while_cond
  %10 = load i8, ptr %i, align 1
  %11 = getelementptr i64, ptr %0, i8 %10
  %12 = load i64, ptr %11, align 8
  store i64 %12, ptr %k, align 8
  %13 = load i64, ptr %k, align 8
  %14 = mul i64 %13, -4132994306676758123
  store i64 %14, ptr %k, align 8
  %15 = load i64, ptr %k, align 8
  %16 = lshr i64 %15, 47
  %17 = load i64, ptr %k, align 8
  %18 = xor i64 %17, %16
  store i64 %18, ptr %k, align 8
  %19 = load i64, ptr %k, align 8
  %20 = mul i64 %19, -4132994306676758123
  store i64 %20, ptr %k, align 8
  %21 = load i64, ptr %k, align 8
  %22 = load i64, ptr %id, align 8
  %23 = xor i64 %22, %21
  store i64 %23, ptr %id, align 8
  %24 = load i64, ptr %id, align 8
  %25 = mul i64 %24, -4132994306676758123
  store i64 %25, ptr %id, align 8
  %26 = load i8, ptr %i, align 1
  %27 = add i8 %26, 1
  store i8 %27, ptr %i, align 1
  br label %while_cond

while_end:                                        ; preds = %while_cond
  call void @llvm.lifetime.end.p0(i64 -1, ptr %nr_stack_frames_addr)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %seed_addr)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %i)
  call void @llvm.lifetime.end.p0(i64 -1, ptr %k)
  %28 = load i64, ptr %id, align 8
  %zero_cond = icmp eq i64 %28, 0
  br i1 %zero_cond, label %if_zero, label %if_end

if_zero:                                          ; preds = %while_end
  store i64 1, ptr %id, align 8
  br label %if_end

if_end:                                           ; preds = %if_zero, %while_end
  %29 = load i64, ptr %id, align 8
  call void @llvm.lifetime.end.p0(i64 -1, ptr %id)
  ret i64 %29
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg %0, ptr nocapture %1) #2

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg %0, ptr nocapture %1) #2

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly %0, i8 %1, i64 %2, i1 immarg %3) #3

attributes #0 = { nounwind }
attributes #1 = { alwaysinline nounwind }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #4 = { memory(none) }

!llvm.dbg.cu = !{!95}
!llvm.module.flags = !{!97, !98}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "LICENSE", linkageName: "global", scope: !2, file: !2, type: !3, isLocal: false, isDefinition: true)
!2 = !DIFile(filename: "bpftrace.bpf.o", directory: ".")
!3 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 32, elements: !5)
!4 = !DIBasicType(name: "int8", size: 8, encoding: DW_ATE_signed)
!5 = !{!6}
!6 = !DISubrange(count: 4, lowerBound: 0)
!7 = !DIGlobalVariableExpression(var: !8, expr: !DIExpression())
!8 = distinct !DIGlobalVariable(name: "AT_x", linkageName: "global", scope: !2, file: !2, type: !9, isLocal: false, isDefinition: true)
!9 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 256, elements: !10)
!10 = !{!11, !17, !18, !21}
!11 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !13, size: 64)
!13 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 32, elements: !15)
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !{!16}
!16 = !DISubrange(count: 1, lowerBound: 0)
!17 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !2, file: !2, baseType: !12, size: 64, offset: 64)
!18 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !2, file: !2, baseType: !19, size: 64, offset: 128)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !20, size: 64)
!20 = !DIBasicType(name: "int64", size: 64, encoding: DW_ATE_signed)
!21 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !2, file: !2, baseType: !22, size: 64, offset: 192)
!22 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !23, size: 64)
!23 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 192, elements: !24)
!24 = !{!25}
!25 = !DISubrange(count: 24, lowerBound: 0)
!26 = !DIGlobalVariableExpression(var: !27, expr: !DIExpression())
!27 = distinct !DIGlobalVariable(name: "AT_y", linkageName: "global", scope: !2, file: !2, type: !9, isLocal: false, isDefinition: true)
!28 = !DIGlobalVariableExpression(var: !29, expr: !DIExpression())
!29 = distinct !DIGlobalVariable(name: "AT_z", linkageName: "global", scope: !2, file: !2, type: !9, isLocal: false, isDefinition: true)
!30 = !DIGlobalVariableExpression(var: !31, expr: !DIExpression())
!31 = distinct !DIGlobalVariable(name: "stack_perf_127", linkageName: "global", scope: !2, file: !2, type: !32, isLocal: false, isDefinition: true)
!32 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 256, elements: !33)
!33 = !{!34, !39, !44, !49}
!34 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !35, size: 64)
!35 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !36, size: 64)
!36 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 288, elements: !37)
!37 = !{!38}
!38 = !DISubrange(count: 9, lowerBound: 0)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !2, file: !2, baseType: !40, size: 64, offset: 64)
!40 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !41, size: 64)
!41 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 4194304, elements: !42)
!42 = !{!43}
!43 = !DISubrange(count: 131072, lowerBound: 0)
!44 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !2, file: !2, baseType: !45, size: 64, offset: 128)
!45 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !46, size: 64)
!46 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 128, elements: !47)
!47 = !{!48}
!48 = !DISubrange(count: 16, lowerBound: 0)
!49 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !2, file: !2, baseType: !50, size: 64, offset: 192)
!50 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !51, size: 64)
!51 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 8128, elements: !52)
!52 = !{!53}
!53 = !DISubrange(count: 127, lowerBound: 0)
!54 = !DIGlobalVariableExpression(var: !55, expr: !DIExpression())
!55 = distinct !DIGlobalVariable(name: "stack_bpftrace_6", linkageName: "global", scope: !2, file: !2, type: !56, isLocal: false, isDefinition: true)
!56 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 256, elements: !57)
!57 = !{!34, !39, !44, !58}
!58 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !2, file: !2, baseType: !59, size: 64, offset: 192)
!59 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !60, size: 64)
!60 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 384, elements: !61)
!61 = !{!62}
!62 = !DISubrange(count: 6, lowerBound: 0)
!63 = !DIGlobalVariableExpression(var: !64, expr: !DIExpression())
!64 = distinct !DIGlobalVariable(name: "stack_bpftrace_127", linkageName: "global", scope: !2, file: !2, type: !32, isLocal: false, isDefinition: true)
!65 = !DIGlobalVariableExpression(var: !66, expr: !DIExpression())
!66 = distinct !DIGlobalVariable(name: "stack_scratch", linkageName: "global", scope: !2, file: !2, type: !67, isLocal: false, isDefinition: true)
!67 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 256, elements: !68)
!68 = !{!69, !17, !72, !49}
!69 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !70, size: 64)
!70 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !71, size: 64)
!71 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 192, elements: !61)
!72 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !2, file: !2, baseType: !73, size: 64, offset: 128)
!73 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !74, size: 64)
!74 = !DIBasicType(name: "int32", size: 32, encoding: DW_ATE_signed)
!75 = !DIGlobalVariableExpression(var: !76, expr: !DIExpression())
!76 = distinct !DIGlobalVariable(name: "ringbuf", linkageName: "global", scope: !2, file: !2, type: !77, isLocal: false, isDefinition: true)
!77 = !DICompositeType(tag: DW_TAG_structure_type, scope: !2, file: !2, size: 128, elements: !78)
!78 = !{!79, !84}
!79 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !2, file: !2, baseType: !80, size: 64)
!80 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !81, size: 64)
!81 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 864, elements: !82)
!82 = !{!83}
!83 = !DISubrange(count: 27, lowerBound: 0)
!84 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !2, file: !2, baseType: !85, size: 64, offset: 64)
!85 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !86, size: 64)
!86 = !DICompositeType(tag: DW_TAG_array_type, baseType: !14, size: 8388608, elements: !87)
!87 = !{!88}
!88 = !DISubrange(count: 262144, lowerBound: 0)
!89 = !DIGlobalVariableExpression(var: !90, expr: !DIExpression())
!90 = distinct !DIGlobalVariable(name: "__bt__event_loss_counter", linkageName: "global", scope: !2, file: !2, type: !91, isLocal: false, isDefinition: true)
!91 = !DICompositeType(tag: DW_TAG_array_type, baseType: !92, size: 64, elements: !15)
!92 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 64, elements: !15)
!93 = !DIGlobalVariableExpression(var: !94, expr: !DIExpression())
!94 = distinct !DIGlobalVariable(name: "__bt__max_cpu_id", linkageName: "global", scope: !2, file: !2, type: !20, isLocal: false, isDefinition: true)
!95 = distinct !DICompileUnit(language: DW_LANG_C, file: !2, producer: "bpftrace", isOptimized: false, runtimeVersion: 0, emissionKind: LineTablesOnly, globals: !96)
!96 = !{!0, !7, !26, !28, !30, !54, !63, !65, !75, !89, !93}
!97 = !{i32 2, !"Debug Info Version", i32 3}
!98 = !{i32 7, !"uwtable", i32 0}
!99 = distinct !DISubprogram(name: "kprobe_f_1", linkageName: "kprobe_f_1", scope: !2, file: !2, type: !100, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !95, retainedNodes: !103)
!100 = !DISubroutineType(types: !101)
!101 = !{!20, !102}
!102 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 64)
!103 = !{!104}
!104 = !DILocalVariable(name: "ctx", arg: 1, scope: !99, file: !2, type: !102)
