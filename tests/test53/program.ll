; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [8 x i8] c"Z: %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"Z2: %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [9 x i8] c"Z3: %ld\0A\00", align 1
@.str.3 = private unnamed_addr constant [10 x i8] c"Z21: %ld\0A\00", align 1
@.str.4 = private unnamed_addr constant [11 x i8] c"Z211: %ld\0A\00", align 1
@.str.5 = private unnamed_addr constant [13 x i8] c"Result = %d\0A\00", align 1
@.str.6 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @my_f(i8*, i8*, i8*, i32) local_unnamed_addr #0 {
  br label %5

; <label>:5:                                      ; preds = %24, %4
  %6 = phi i32 [ %27, %24 ], [ 1, %4 ]
  %7 = phi i8* [ %39, %24 ], [ %2, %4 ]
  %8 = phi i32 [ %25, %24 ], [ 0, %4 ]
  %9 = tail call i64 @CAT_get(i8* %7) #3
  %10 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %9)
  tail call void @CAT_add(i8* %7, i8* %0, i8* %1) #3
  %11 = tail call i64 @CAT_get(i8* %7) #3
  %12 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %11)
  tail call void @CAT_add(i8* %0, i8* %0, i8* %0) #3
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %13 = tail call i8* @CAT_new(i64 1) #3
  br label %14

; <label>:14:                                     ; preds = %14, %5
  %15 = phi i32 [ 0, %5 ], [ %22, %14 ]
  %16 = phi i8* [ %13, %5 ], [ %21, %14 ]
  %17 = tail call i64 @CAT_get(i8* %16) #3
  %18 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %17)
  tail call void @CAT_add(i8* %16, i8* %0, i8* %1) #3
  %19 = tail call i64 @CAT_get(i8* %16) #3
  %20 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %19)
  tail call void @CAT_add(i8* %0, i8* %0, i8* %0) #3
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %21 = tail call i8* @CAT_new(i64 1) #3
  %22 = add nuw nsw i32 %15, 1
  %23 = icmp eq i32 %22, 10
  br i1 %23, label %28, label %14

; <label>:24:                                     ; preds = %38
  %25 = add nuw nsw i32 %8, 1
  %26 = icmp sgt i32 %8, %3
  %27 = add nuw i32 %6, 1
  br i1 %26, label %74, label %5

; <label>:28:                                     ; preds = %14, %38
  %29 = phi i32 [ %40, %38 ], [ 0, %14 ]
  %30 = phi i8* [ %39, %38 ], [ %21, %14 ]
  %31 = tail call i64 @CAT_get(i8* %30) #3
  %32 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %31)
  tail call void @CAT_add(i8* %30, i8* %0, i8* %1) #3
  %33 = tail call i64 @CAT_get(i8* %30) #3
  %34 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %33)
  tail call void @CAT_add(i8* %0, i8* %0, i8* %0) #3
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %35 = tail call i8* @CAT_new(i64 1) #3
  %36 = mul nsw i32 %29, 3
  %37 = icmp eq i32 %29, 0
  br i1 %37, label %38, label %42

; <label>:38:                                     ; preds = %60, %28
  %39 = phi i8* [ %35, %28 ], [ %61, %60 ]
  %40 = add nuw nsw i32 %29, 1
  %41 = icmp eq i32 %40, %6
  br i1 %41, label %24, label %28

; <label>:42:                                     ; preds = %28, %60
  %43 = phi i32 [ %62, %60 ], [ 0, %28 ]
  %44 = phi i8* [ %61, %60 ], [ %35, %28 ]
  %45 = tail call i64 @CAT_get(i8* %44) #3
  %46 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %45)
  tail call void @CAT_add(i8* %44, i8* %0, i8* %1) #3
  %47 = tail call i64 @CAT_get(i8* %44) #3
  %48 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %47)
  tail call void @CAT_add(i8* %0, i8* %0, i8* %0) #3
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %49 = tail call i8* @CAT_new(i64 1) #3
  %50 = tail call i64 @CAT_get(i8* %0) #3
  %51 = icmp sgt i64 %50, 1000000
  br i1 %51, label %52, label %56

; <label>:52:                                     ; preds = %42
  %53 = tail call i32 @my_f(i8* %0, i8* %1, i8* %49, i32 %3)
  %54 = sext i32 %53 to i64
  %55 = tail call i8* @CAT_new(i64 %54) #3
  br label %56

; <label>:56:                                     ; preds = %52, %42
  %57 = phi i8* [ %55, %52 ], [ %49, %42 ]
  %58 = shl nuw nsw i32 %43, 1
  %59 = icmp eq i32 %43, 0
  br i1 %59, label %60, label %64

; <label>:60:                                     ; preds = %64, %56
  %61 = phi i8* [ %57, %56 ], [ %71, %64 ]
  %62 = add nuw nsw i32 %43, 1
  %63 = icmp ult i32 %62, %36
  br i1 %63, label %42, label %38

; <label>:64:                                     ; preds = %56, %64
  %65 = phi i32 [ %72, %64 ], [ 0, %56 ]
  %66 = phi i8* [ %71, %64 ], [ %57, %56 ]
  %67 = tail call i64 @CAT_get(i8* %66) #3
  %68 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %67)
  tail call void @CAT_add(i8* %66, i8* %0, i8* %1) #3
  %69 = tail call i64 @CAT_get(i8* %66) #3
  %70 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %69)
  tail call void @CAT_add(i8* %0, i8* %0, i8* %0) #3
  tail call void @CAT_add(i8* %1, i8* %1, i8* %1) #3
  %71 = tail call i8* @CAT_new(i64 1) #3
  %72 = add nuw nsw i32 %65, 1
  %73 = icmp slt i32 %72, %58
  br i1 %73, label %64, label %60

; <label>:74:                                     ; preds = %24
  %75 = tail call i64 @CAT_get(i8* %39) #3
  %76 = trunc i64 %75 to i32
  ret i32 %76
}

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #1

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #2

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #2

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #2

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  %3 = tail call i8* @CAT_new(i64 40) #3
  %4 = tail call i8* @CAT_new(i64 2) #3
  %5 = tail call i8* @CAT_new(i64 0) #3
  %6 = tail call i32 @my_f(i8* %3, i8* %4, i8* %5, i32 %0)
  %7 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str.5, i64 0, i64 0), i32 %6)
  %8 = tail call i64 @CAT_invocations() #3
  %9 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.6, i64 0, i64 0), i64 %8)
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
