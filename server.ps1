$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://127.0.0.1:8080/")
$listener.Start()
Write-Host "PowerShell Web Server running at http://127.0.0.1:8080/"

try {
    while ($listener.IsListening) {
        $context = $listener.GetContext()
        $request = $context.Request
        $response = $context.Response
        
        $url = $request.Url.LocalPath
        if ($url -eq "/") { $url = "/index.html" }
        
        # Clean the file path
        $cleanUrl = $url.Replace("/", "\").TrimStart("\")
        $file = Join-Path "c:\__repo\games\dot blaster\game" $cleanUrl
        
        if (Test-Path $file -PathType Leaf) {
            $bytes = [System.IO.File]::ReadAllBytes($file)
            
            # Set content type
            if ($file.EndsWith(".html")) {
                $response.ContentType = "text/html"
            } elseif ($file.EndsWith(".js")) {
                $response.ContentType = "application/javascript"
            } elseif ($file.EndsWith(".nes")) {
                $response.ContentType = "application/octet-stream"
            }
            
            $response.ContentLength64 = $bytes.Length
            $response.OutputStream.Write($bytes, 0, $bytes.Length)
        } else {
            $response.StatusCode = 404
            $errBytes = [System.Text.Encoding]::UTF8.GetBytes("File Not Found")
            $response.OutputStream.Write($errBytes, 0, $errBytes.Length)
        }
        $response.OutputStream.Close()
    }
} finally {
    $listener.Stop()
}
