; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [20 x i8] c"H1: \09Value 1 = %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [20 x i8] c"H1: \09Value 2 = %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [20 x i8] c"H1: \09Value 3 = %ld\0A\00", align 1
@.str.3 = private unnamed_addr constant [20 x i8] c"H1: \09Value 4 = %ld\0A\00", align 1
@.str.4 = private unnamed_addr constant [20 x i8] c"H1: \09Value 5 = %ld\0A\00", align 1
@.str.5 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local void @CAT_execution(i32) local_unnamed_addr #0 {
  %2 = tail call i8* @CAT_new(i64 5) #3
  %3 = tail call i8* @CAT_new(i64 8) #3
  %4 = tail call i8* @CAT_new(i64 8) #3
  %5 = tail call i8* @CAT_new(i64 8) #3
  %6 = icmp sgt i32 %0, 10
  br i1 %6, label %7, label %8

; <label>:7:                                      ; preds = %1
  tail call void @CAT_add(i8* %2, i8* %3, i8* %4) #3
  br label %8

; <label>:8:                                      ; preds = %7, %1
  %9 = tail call i64 @CAT_get(i8* %2) #3
  %10 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i64 0, i64 0), i64 %9)
  %11 = tail call i64 @CAT_get(i8* %3) #3
  %12 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %11)
  %13 = tail call i64 @CAT_get(i8* %4) #3
  %14 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.2, i64 0, i64 0), i64 %13)
  %15 = tail call i64 @CAT_get(i8* %5) #3
  %16 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.3, i64 0, i64 0), i64 %15)
  tail call void @CAT_add(i8* %2, i8* %3, i8* %4) #3
  %17 = tail call i64 @CAT_get(i8* %2) #3
  %18 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i64 0, i64 0), i64 %17)
  %19 = tail call i64 @CAT_get(i8* %3) #3
  %20 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %19)
  %21 = tail call i64 @CAT_get(i8* %4) #3
  %22 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.2, i64 0, i64 0), i64 %21)
  %23 = tail call i64 @CAT_get(i8* %5) #3
  %24 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.3, i64 0, i64 0), i64 %23)
  %25 = icmp sgt i32 %0, 20
  br i1 %25, label %26, label %28

; <label>:26:                                     ; preds = %8
  %27 = tail call i8* @CAT_new(i64 0) #3
  br label %28

; <label>:28:                                     ; preds = %26, %8
  %29 = tail call i64 @CAT_get(i8* %2) #3
  %30 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i64 0, i64 0), i64 %29)
  %31 = tail call i64 @CAT_get(i8* %3) #3
  %32 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.1, i64 0, i64 0), i64 %31)
  %33 = tail call i64 @CAT_get(i8* %4) #3
  %34 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.2, i64 0, i64 0), i64 %33)
  %35 = tail call i64 @CAT_get(i8* %5) #3
  %36 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.3, i64 0, i64 0), i64 %35)
  %37 = icmp sgt i32 %0, 25
  br i1 %37, label %38, label %41

; <label>:38:                                     ; preds = %28
  %39 = tail call i64 @CAT_get(i8* %5) #3
  %40 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str.4, i64 0, i64 0), i64 %39)
  br label %41

; <label>:41:                                     ; preds = %38, %28
  ret void
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
  %4 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.5, i64 0, i64 0), i64 %3)
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
