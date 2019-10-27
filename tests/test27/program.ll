; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local void @a_generic_C_function(i8*, i32) local_unnamed_addr #0 {
  %3 = icmp slt i32 %1, 100
  br i1 %3, label %4, label %5

; <label>:4:                                      ; preds = %16, %2
  ret void

; <label>:5:                                      ; preds = %2, %16
  %6 = phi i32 [ %18, %16 ], [ %1, %2 ]
  %7 = phi i8* [ %21, %16 ], [ %0, %2 ]
  %8 = tail call i64 @CAT_get(i8* %7) #3
  %9 = icmp sgt i64 %8, 10
  br i1 %9, label %10, label %14

; <label>:10:                                     ; preds = %5
  %11 = tail call i64 @CAT_get(i8* %7) #3
  %12 = add nsw i64 %11, -1
  %13 = tail call i8* @CAT_new(i64 %12) #3
  br label %16

; <label>:14:                                     ; preds = %5
  %15 = add nsw i32 %6, -1
  br label %16

; <label>:16:                                     ; preds = %14, %10
  %17 = phi i8* [ %13, %10 ], [ %7, %14 ]
  %18 = phi i32 [ %6, %10 ], [ %15, %14 ]
  %19 = tail call i64 @CAT_get(i8* %17) #3
  %20 = add nsw i64 %19, -1
  %21 = tail call i8* @CAT_new(i64 %20) #3
  %22 = icmp slt i32 %18, 100
  br i1 %22, label %4, label %5
}

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #1

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  %3 = tail call i8* @CAT_new(i64 5) #3
  tail call void @a_generic_C_function(i8* %3, i32 %0)
  %4 = tail call i64 @CAT_invocations() #3
  %5 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str, i64 0, i64 0), i64 %4)
  ret i32 0
}

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #2

declare dso_local i64 @CAT_invocations() local_unnamed_addr #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
