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
  br label %9

; <label>:6:                                      ; preds = %27
  %7 = tail call i64 @CAT_invocations() #3
  %8 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.5, i64 0, i64 0), i64 %7)
  ret i32 0

; <label>:9:                                      ; preds = %27, %2
  %10 = phi i32 [ 1, %2 ], [ %28, %27 ]
  %11 = phi i8* [ %5, %2 ], [ %41, %27 ]
  %12 = tail call i64 @CAT_get(i8* %11) #3
  %13 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %12)
  tail call void @CAT_add(i8* %11, i8* %3, i8* %4) #3
  %14 = tail call i64 @CAT_get(i8* %11) #3
  %15 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0), i64 %14)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %16 = tail call i8* @CAT_new(i64 1) #3
  br label %17

; <label>:17:                                     ; preds = %17, %9
  %18 = phi i32 [ 0, %9 ], [ %25, %17 ]
  %19 = phi i8* [ %16, %9 ], [ %24, %17 ]
  %20 = tail call i64 @CAT_get(i8* %19) #3
  %21 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %20)
  tail call void @CAT_add(i8* %19, i8* %3, i8* %4) #3
  %22 = tail call i64 @CAT_get(i8* %19) #3
  %23 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i64 0, i64 0), i64 %22)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %24 = tail call i8* @CAT_new(i64 1) #3
  %25 = add nuw nsw i32 %18, 1
  %26 = icmp eq i32 %25, 10
  br i1 %26, label %30, label %17

; <label>:27:                                     ; preds = %40
  %28 = add nuw nsw i32 %10, 1
  %29 = icmp eq i32 %28, 11
  br i1 %29, label %6, label %9

; <label>:30:                                     ; preds = %17, %40
  %31 = phi i32 [ %42, %40 ], [ 0, %17 ]
  %32 = phi i8* [ %41, %40 ], [ %24, %17 ]
  %33 = tail call i64 @CAT_get(i8* %32) #3
  %34 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %33)
  tail call void @CAT_add(i8* %32, i8* %3, i8* %4) #3
  %35 = tail call i64 @CAT_get(i8* %32) #3
  %36 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.2, i64 0, i64 0), i64 %35)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %37 = tail call i8* @CAT_new(i64 1) #3
  %38 = mul nuw nsw i32 %31, 3
  %39 = icmp eq i32 %31, 0
  br i1 %39, label %40, label %44

; <label>:40:                                     ; preds = %54, %30
  %41 = phi i8* [ %37, %30 ], [ %55, %54 ]
  %42 = add nuw nsw i32 %31, 1
  %43 = icmp eq i32 %42, %10
  br i1 %43, label %27, label %30

; <label>:44:                                     ; preds = %30, %54
  %45 = phi i32 [ %56, %54 ], [ 0, %30 ]
  %46 = phi i8* [ %55, %54 ], [ %37, %30 ]
  %47 = tail call i64 @CAT_get(i8* %46) #3
  %48 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %47)
  tail call void @CAT_add(i8* %46, i8* %3, i8* %4) #3
  %49 = tail call i64 @CAT_get(i8* %46) #3
  %50 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str.3, i64 0, i64 0), i64 %49)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %51 = tail call i8* @CAT_new(i64 1) #3
  %52 = shl nuw nsw i32 %45, 1
  %53 = icmp eq i32 %45, 0
  br i1 %53, label %54, label %58

; <label>:54:                                     ; preds = %58, %44
  %55 = phi i8* [ %51, %44 ], [ %65, %58 ]
  %56 = add nuw nsw i32 %45, 1
  %57 = icmp ult i32 %56, %38
  br i1 %57, label %44, label %40

; <label>:58:                                     ; preds = %44, %58
  %59 = phi i32 [ %66, %58 ], [ 0, %44 ]
  %60 = phi i8* [ %65, %58 ], [ %51, %44 ]
  %61 = tail call i64 @CAT_get(i8* %60) #3
  %62 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %61)
  tail call void @CAT_add(i8* %60, i8* %3, i8* %4) #3
  %63 = tail call i64 @CAT_get(i8* %60) #3
  %64 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.4, i64 0, i64 0), i64 %63)
  tail call void @CAT_add(i8* %3, i8* %3, i8* %3) #3
  tail call void @CAT_add(i8* %4, i8* %4, i8* %4) #3
  %65 = tail call i8* @CAT_new(i64 1) #3
  %66 = add nuw nsw i32 %59, 1
  %67 = icmp ult i32 %66, %52
  br i1 %67, label %58, label %54
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
