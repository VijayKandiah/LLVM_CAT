; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [10 x i8] c"HELLO %f\0A\00", align 1
@you_cannot_take_decisions_based_on_function_names.internalD1 = internal global i8* null, align 8
@.str.6 = private unnamed_addr constant [13 x i8] c"Values: %ld\0A\00", align 1
@.str.7 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1
@str = private unnamed_addr constant [4 x i8] c"WOW\00", align 1
@str.8 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@str.9 = private unnamed_addr constant [8 x i8] c"Hello 2\00", align 1
@str.10 = private unnamed_addr constant [26 x i8] c"This block doesn't matter\00", align 1
@str.11 = private unnamed_addr constant [20 x i8] c"Invoking a function\00", align 1
@str.12 = private unnamed_addr constant [26 x i8] c"This block doesn't matter\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @another_function(i32, float, double) local_unnamed_addr #0 {
  %4 = sitofp i32 %0 to double
  %5 = fpext float %1 to double
  %6 = fadd double %4, %5
  %7 = fadd double %6, %2
  %8 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str, i64 0, i64 0), double %7)
  %9 = add nsw i32 %0, 100
  ret i32 %9
}

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @yet_another_function(i8** readnone, i8*) local_unnamed_addr #0 {
  %3 = icmp eq i8** %0, null
  br i1 %3, label %5, label %4

; <label>:4:                                      ; preds = %2
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #4
  br label %7

; <label>:5:                                      ; preds = %2
  %6 = tail call i32 @puts(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0))
  br label %7

; <label>:7:                                      ; preds = %5, %4
  ret void
}

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #2

; Function Attrs: norecurse nounwind uwtable
define dso_local i8** @you_cannot_take_decisions_based_on_function_names(i8*) local_unnamed_addr #3 {
  %2 = icmp eq i8* %0, null
  br i1 %2, label %7, label %3

; <label>:3:                                      ; preds = %1
  %4 = load i8*, i8** @you_cannot_take_decisions_based_on_function_names.internalD1, align 8, !tbaa !2
  %5 = icmp eq i8* %4, null
  br i1 %5, label %6, label %7

; <label>:6:                                      ; preds = %3
  store i8* %0, i8** @you_cannot_take_decisions_based_on_function_names.internalD1, align 8, !tbaa !2
  br label %7

; <label>:7:                                      ; preds = %3, %6, %1
  %8 = phi i8** [ null, %1 ], [ @you_cannot_take_decisions_based_on_function_names.internalD1, %6 ], [ @you_cannot_take_decisions_based_on_function_names.internalD1, %3 ]
  ret i8** %8
}

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  %3 = tail call i8* @CAT_new(i64 5) #4
  %4 = ptrtoint i8* %3 to i64
  %5 = icmp sgt i32 %0, 5
  br i1 %5, label %6, label %8

; <label>:6:                                      ; preds = %2
  %7 = tail call i32 @puts(i8* getelementptr inbounds ([26 x i8], [26 x i8]* @str.12, i64 0, i64 0))
  br label %15

; <label>:8:                                      ; preds = %2
  %9 = shl i64 %4, 32
  %10 = ashr exact i64 %9, 32
  %11 = inttoptr i64 %10 to i8*
  %12 = tail call i8** @you_cannot_take_decisions_based_on_function_names(i8* %11)
  %13 = tail call i32 @puts(i8* getelementptr inbounds ([6 x i8], [6 x i8]* @str.8, i64 0, i64 0))
  %14 = bitcast i8** %12 to i8***
  br label %15

; <label>:15:                                     ; preds = %8, %6
  %16 = phi i8*** [ null, %6 ], [ %14, %8 ]
  %17 = tail call i32 @puts(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @str.9, i64 0, i64 0))
  %18 = add nsw i32 %0, 1
  %19 = sitofp i32 %18 to float
  %20 = tail call i32 @another_function(i32 %0, float %19, double 4.242000e+01)
  %21 = icmp sgt i32 %20, 10
  br i1 %21, label %22, label %28

; <label>:22:                                     ; preds = %15
  %23 = tail call i32 @puts(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @str.11, i64 0, i64 0))
  %24 = load i8**, i8*** %16, align 8, !tbaa !2
  %25 = shl i64 %4, 32
  %26 = ashr exact i64 %25, 32
  %27 = inttoptr i64 %26 to i8*
  tail call void @yet_another_function(i8** %24, i8* %27)
  br label %30

; <label>:28:                                     ; preds = %15
  %29 = tail call i32 @puts(i8* getelementptr inbounds ([26 x i8], [26 x i8]* @str.10, i64 0, i64 0))
  br label %30

; <label>:30:                                     ; preds = %28, %22
  %31 = shl i64 %4, 32
  %32 = ashr exact i64 %31, 32
  %33 = inttoptr i64 %32 to i8*
  %34 = tail call i64 @CAT_get(i8* %33) #4
  %35 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str.6, i64 0, i64 0), i64 %34)
  %36 = tail call i64 @CAT_invocations() #4
  %37 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.7, i64 0, i64 0), i64 %36)
  ret i32 0
}

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #2

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #2

declare dso_local i64 @CAT_invocations() local_unnamed_addr #2

; Function Attrs: nounwind
declare i32 @puts(i8* nocapture readonly) local_unnamed_addr #4

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
