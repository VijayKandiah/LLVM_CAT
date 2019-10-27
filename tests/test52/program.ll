; ModuleID = 'program.bc'
source_filename = "program.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [8 x i8] c"Z: %ld\0A\00", align 1
@.str.1 = private unnamed_addr constant [9 x i8] c"Z2: %ld\0A\00", align 1
@.str.2 = private unnamed_addr constant [9 x i8] c"Z3: %ld\0A\00", align 1
@.str.3 = private unnamed_addr constant [10 x i8] c"Z21: %ld\0A\00", align 1
@.str.4 = private unnamed_addr constant [11 x i8] c"Z211: %ld\0A\00", align 1
@.str.5 = private unnamed_addr constant [23 x i8] c"CAT invocations = %ld\0A\00", align 1

; Function Attrs: nounwind uwtable
define dso_local i32 @main(i32, i8** nocapture readnone) local_unnamed_addr #0 {
  %3 = tail call i8* @CAT_new(i64 40) #3
  %4 = tail call i8* @CAT_new(i64 2) #3
  %5 = tail call i8* @CAT_new(i64 0) #3
  br label %6

; <label>:6:                                      ; preds = %25, %2
  %7 = phi i32 [ %28, %25 ], [ 1, %2 ]
  %8 = phi i8* [ %40, %25 ], [ %5, %2 ]
  %9 = phi i32 [ %26, %25 ], [ 0, %2 ]
  %10 = tail call i64 @CAT_get(i8* %8) #3
  %11 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %10)
  tail call void @CAT_add(i8* %8, i8* %3, i8* %4) #3
  %12 = tail call i64 @CAT_get(i8* %8) #3
  %13 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %12)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %14 = tail call i8* @CAT_new(i64 1) #3
  br label %15

; <label>:15:                                     ; preds = %15, %6
  %16 = phi i32 [ 0, %6 ], [ %23, %15 ]
  %17 = phi i8* [ %14, %6 ], [ %22, %15 ]
  %18 = tail call i64 @CAT_get(i8* %17) #3
  %19 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %18)
  tail call void @CAT_add(i8* %17, i8* %3, i8* %4) #3
  %20 = tail call i64 @CAT_get(i8* %17) #3
  %21 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %20)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %22 = tail call i8* @CAT_new(i64 1) #3
  %23 = add nuw nsw i32 %16, 1
  %24 = icmp eq i32 %23, 10
  br i1 %24, label %29, label %15

; <label>:25:                                     ; preds = %39
  %26 = add nuw nsw i32 %9, 1
  %27 = icmp sgt i32 %9, %0
  %28 = add nuw i32 %7, 1
  br i1 %27, label %67, label %6

; <label>:29:                                     ; preds = %15, %39
  %30 = phi i32 [ %41, %39 ], [ 0, %15 ]
  %31 = phi i8* [ %40, %39 ], [ %22, %15 ]
  %32 = tail call i64 @CAT_get(i8* %31) #3
  %33 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %32)
  tail call void @CAT_add(i8* %31, i8* %3, i8* %4) #3
  %34 = tail call i64 @CAT_get(i8* %31) #3
  %35 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %34)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %36 = tail call i8* @CAT_new(i64 1) #3
  %37 = mul nsw i32 %30, 3
  %38 = icmp eq i32 %30, 0
  br i1 %38, label %39, label %43

; <label>:39:                                     ; preds = %53, %29
  %40 = phi i8* [ %36, %29 ], [ %54, %53 ]
  %41 = add nuw nsw i32 %30, 1
  %42 = icmp eq i32 %41, %7
  br i1 %42, label %25, label %29

; <label>:43:                                     ; preds = %29, %53
  %44 = phi i32 [ %55, %53 ], [ 0, %29 ]
  %45 = phi i8* [ %54, %53 ], [ %36, %29 ]
  %46 = tail call i64 @CAT_get(i8* %45) #3
  %47 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %46)
  tail call void @CAT_add(i8* %45, i8* %3, i8* %4) #3
  %48 = tail call i64 @CAT_get(i8* %45) #3
  %49 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %48)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %50 = tail call i8* @CAT_new(i64 1) #3
  %51 = shl nuw nsw i32 %44, 1
  %52 = icmp eq i32 %44, 0
  br i1 %52, label %53, label %57

; <label>:53:                                     ; preds = %57, %43
  %54 = phi i8* [ %50, %43 ], [ %64, %57 ]
  %55 = add nuw nsw i32 %44, 1
  %56 = icmp ult i32 %55, %37
  br i1 %56, label %43, label %39

; <label>:57:                                     ; preds = %43, %57
  %58 = phi i32 [ %65, %57 ], [ 0, %43 ]
  %59 = phi i8* [ %64, %57 ], [ %50, %43 ]
  %60 = tail call i64 @CAT_get(i8* %59) #3
  %61 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %60)
  tail call void @CAT_add(i8* %59, i8* %3, i8* %4) #3
  %62 = tail call i64 @CAT_get(i8* %59) #3
  %63 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %62)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %64 = tail call i8* @CAT_new(i64 1) #3
  %65 = add nuw nsw i32 %58, 1
  %66 = icmp slt i32 %65, %51
  br i1 %66, label %57, label %53

; <label>:67:                                     ; preds = %25
  %68 = tail call i64 @CAT_invocations() #3
  %69 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.5, i64 0, i64 0), i64 %68)
  ret i32 0
}

declare dso_local i8* @CAT_new(i64) local_unnamed_addr #1

; Function Attrs: nounwind
declare dso_local i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #2

declare dso_local i64 @CAT_get(i8*) local_unnamed_addr #1

declare dso_local void @CAT_add(i8*, i8*, i8*) local_unnamed_addr #1

declare dso_local i64 @CAT_invocations() local_unnamed_addr #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 8.0.0 (tags/RELEASE_800/final)"}
