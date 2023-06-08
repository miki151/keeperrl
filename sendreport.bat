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
ren report.txt crashreport%time:~0,2%%time:~3,2%-%date:~-10,2%%date:~3,2%%date:~-4,4%.txt
@del stacktrace.out
