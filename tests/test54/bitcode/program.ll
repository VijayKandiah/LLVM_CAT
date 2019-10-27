; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [9 x i8] c"L1: %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"L2: %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [16 x i8] c"Result = %d\0A\0A\0A\0A\00", align 1
@.str.3 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: norecurse nounwind readnone uwtable
define dso_local i8* @myF(i8* readnone returned) local_unnamed_addr #0 {
  ret i8* %0
}

; Function Attrs: nounwind uwtable
define dso_local void @computeF() local_unnamed_addr #1 {
  %1 = tail call i8* @CAT_new(i64 4) #4
  %2 = tail call i8* @CAT_new(i64 42) #4
  %3 = tail call i64 @CAT_get(i8* %1) #4
  %4 = trunc i64 %3 to i32
  %5 = icmp sgt i32 %4, 9
  br i1 %5, label %6, label %9

; <label>:6:                                      ; preds = %0
  %7 = tail call i64 @CAT_get(i8* %1) #4
  %8 = tail call i8* @CAT_new(i64 %7) #4
  br label %14

; <label>:9:                                      ; preds = %0
  %10 = tail call i8* @CAT_new(i64 5) #4
  %11 = tail call i8* @myF(i8* %10)
  %12 = tail call i64 @CAT_get(i8* %11) #4
  %13 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str, i64 0, i64 0), i64 %12)
  br label %14

; <label>:14:                                     ; preds = %9, %6
  %15 = phi i8* [ %8, %6 ], [ %11, %9 ]
  tail call void @CAT_add(i8* %1, i8* %1, i8* %15) #4
  %16 = tail call i64 @CAT_get(i8* %2) #4
  br label %17

; <label>:17:                                     ; preds = %14, %36
  %18 = phi i32 [ 0, %14 ], [ %37, %36 ]
  %19 = tail call i64 @CAT_get(i8* %1) #4
  %20 = trunc i64 %19 to i32
  %21 = icmp sgt i32 %20, 9
  br i1 %21, label %22, label %25

; <label>:22:                                     ; preds = %17
  %23 = tail call i64 @CAT_get(i8* %1) #4
  %24 = tail call i8* @CAT_new(i64 %23) #4
  br label %30

; <label>:25:                                     ; preds = %17
  %26 = tail call i8* @CAT_new(i64 5) #4
  %27 = tail call i8* @myF(i8* %26)
  %28 = tail call i64 @CAT_get(i8* %27) #4
  %29 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %28)
  br label %30

; <label>:30:                                     ; preds = %22, %25
  %31 = phi i8* [ %24, %22 ], [ %27, %25 ]
  tail call void @CAT_add(i8* %1, i8* %1, i8* %31) #4
  %32 = tail call i64 @CAT_get(i8* %2) #4
  %33 = icmp eq i32 %18, 0
  br i1 %33, label %34, label %36

; <label>:34:                                     ; preds = %30
  %35 = tail call i8* @CAT_new(i64 125) #4
  br label %36

; <label>:36:                                     ; preds = %34, %30
  %37 = add nuw nsw i32 %18, 1
  %38 = icmp eq i32 %37, 10
  br i1 %38, label %39, label %17

; <label>:39:                                     ; preds = %36
  %40 = trunc i64 %32 to i32
  %41 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([16 x i8], [16 x i8]* @.str.2, i64 0, i64 0), i32 %40)
  ret void
}

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #2

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #2

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #3

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #2

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #1 {
  tail call void @computeF()
  %3 = tail call i64 @CAT_invocations() #4
  %4 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.3, i64 0, i64 0), i64 %3)
  ret i32 0
}

declare dso_local i64 @CAT_invocations() local_unnamed_addr #2

attributes #0 = { norecurse nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
