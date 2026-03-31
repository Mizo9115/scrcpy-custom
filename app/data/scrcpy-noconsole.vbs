strCommand = "cmd /c scrcpy.exe --turn-screen-off --screen-off-key --stay-awake --no-audio"

For Each Arg In WScript.Arguments
    strCommand = strCommand & " """ & replace(Arg, """", """""""""") & """"
Next

CreateObject("Wscript.Shell").Run strCommand, 0, false
