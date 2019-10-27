; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [20 x i8] c"H1: \09Value 2 = %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [19 x i8] c"H1: \09Result = %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [20 x i8] c"H1: \09Value 1 = %ld\0A\00", align 1
@.str.3 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local void @CAT_execution(i32) local_unnamed_addr #0 {
  %2 = icmp slt i32 %0, 100
  br i1 %2, label %3, label %8

; <label>:3:                                      ; preds = %1
  %4 = tail call i8* @CAT_new(i64 5) #3
  %5 = tail call i64 @CAT_get(i8* %4) #3
  %6 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.2, i64 0, i64 0), i64 %5)
  %7 = icmp sgt i32 %0, 0
  br i1 %7, label %8, label %12

; <label>:8:                                      ; preds = %1, %3
  %9 = phi i8* [ %4, %3 ], [ undef, %1 ]
  %10 = icmp sgt i32 %0, 10
  %11 = icmp sgt i32 %0, 20
  br label %13

; <label>:12:                                     ; preds = %22, %3
  ret void

; <label>:13:                                     ; preds = %22, %8
  %14 = phi i32 [ 0, %8 ], [ %25, %22 ]
  %15 = tail call i8* @CAT_new(i64 8) #3
  br i1 %10, label %16, label %17

; <label>:16:                                     ; preds = %13
  tail call void @CAT_add(i8* %15, i8* %15, i8* %15) #3
  br label %17

; <label>:17:                                     ; preds = %16, %13
  %18 = tail call i64 @CAT_get(i8* %15) #3
  %19 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i64 0, i64 0), i64 %18)
  %20 = tail call i8* @CAT_new(i64 0) #3
  tail call void @CAT_add(i8* %20, i8* %9, i8* %15) #3
  tail call void @CAT_add(i8* %20, i8* %9, i8* %20) #3
  tail call void @CAT_add(i8* %20, i8* %20, i8* %20) #3
  br i1 %11, label %21, label %22

; <label>:21:                                     ; preds = %17
  tail call void @CAT_add(i8* %20, i8* %9, i8* %9) #3
  br label %22

; <label>:22:                                     ; preds = %21, %17
  %23 = tail call i64 @CAT_get(i8* %20) #3
  %24 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @.str.1, i64 0, i64 0), i64 %23)
  %25 = add nuw nsw i32 %14, 1
  %26 = icmp eq i32 %25, %0
  br i1 %26, label %12, label %13
}

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #1

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #1

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #2

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  tail call void @CAT_execution(i32 %0)
  %3 = tail call i64 @CAT_invocations() #3
  %4 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.3, i64 0, i64 0), i64 %3)
  ret i32 0
}

declare dso_local i64 @CAT_invocations() local_unnamed_addr #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
