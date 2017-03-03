if exist installId.txt (
  @type installId.txt >> report.txt
)
echo. >> report.txt
if exist system_info.txt (
  @type system_info.txt >> report.txt
) else (
  echo. >> report.txt
)
@type stacktrace.out >> report.txt
curl.exe --progress-bar -F "userfile=@report.txt" blkrs.com/~michal/upload_stacktrace.php
@del report.txt
@del stacktrace.out
