# Create a file and delete it. Observe the kernel logs to see if we campture the debug log.

$tempfile = New-TemporaryFile

Start-Sleep -s 1

# now delete the file.
$tempfile.Delete()
