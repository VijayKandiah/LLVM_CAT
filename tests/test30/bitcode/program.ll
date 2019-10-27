; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [14 x i8] c"H1: \09Y = %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [20 x i8] c"H1: \09Value 2 = %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [20 x i8] c"H1: \09Value 1 = %ld\0A\00", align 1
@.str.3 = private unnamed_addr constant [15 x i8] c"Result 2 = %d\0A\00", align 1
@.str.4 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @another_execution(i32, i8*) local_unnamed_addr #0 {
  %3 = tail call i64 @CAT_get(i8* %1) #3
  %4 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i64 0, i64 0), i64 %3)
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %5 = tail call i64 @CAT_get(i8* %1) #3
  %6 = trunc i64 %5 to i32
  ret i32 %6
}

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #1

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #2

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #2

; Function Attrs: nounwind uwtable
define dso_local void @CAT_execution(i32, i8*, i32) local_unnamed_addr #0 {
  %4 = tail call i8* @CAT_new(i64 8) #3
  %5 = tail call i64 @CAT_get(i8* %4) #3
  %6 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %5)
  %7 = icmp sgt i32 %0, 10
  br i1 %7, label %8, label %10

; <label>:8:                                      ; preds = %3
  %9 = tail call i8* @CAT_new(i64 8) #3
  tail call void @CAT_add(i8* %1, i8* %9, i8* %9) #3
  br label %10

; <label>:10:                                     ; preds = %8, %3
  %11 = phi i8* [ %9, %8 ], [ %4, %3 ]
  %12 = tail call i64 @CAT_get(i8* %11) #3
  %13 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %12)
  %14 = tail call i64 @CAT_get(i8* %1) #3
  %15 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.2, i64 0, i64 0), i64 %14)
  %16 = icmp sgt i32 %2, 0
  br i1 %16, label %17, label %20

; <label>:17:                                     ; preds = %10
  %18 = tail call i64 @CAT_get(i8* %11) #3
  %19 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %18)
  br label %20

; <label>:20:                                     ; preds = %17, %10
  ret void
}

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #2

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  %3 = tail call i8* @CAT_new(i64 8) #3
  %4 = add nsw i32 %0, 1
  tail call void @CAT_execution(i32 %0, i8* %3, i32 %4)
  %5 = tail call i32 @another_execution(i32 undef, i8* %3)
  %6 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str.3, i64 0, i64 0), i32 %5)
  %7 = tail call i64 @CAT_invocations() #3
  %8 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.4, i64 0, i64 0), i64 %7)
  ret i32 0
}

declare dso_local i64 @CAT_invocations() local_unnamed_addr #2

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
