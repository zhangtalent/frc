unit pubFunUnit;
{$MODE objfpc}{$H+}
interface
uses
  Classes, SysUtils, IdSocketHandle, IdIPMCastServer, IdTCPClient,
  StrUtils, IdGlobal, IdContext, ConstUnit, crc32unit;
var
  IdIPMCastServer1: TIdIPMCastServer = nil;
  //IdIPMCastClient1: TIdIPMCastClient = nil;
function GetLocalIP(t: TIdSocketHandles): string;
procedure CastMyMessage(d: string);
procedure Str2List(l: TStrings; d, f: string);
procedure ClientSendFile(fsName, SvrIp: string);
function GetFileFromTcpIp(fsName: string; fsSize: Int64; CTxt: TIdContext; Crc: Cardinal = 0): boolean;
procedure GetFileSizeCrc(s: string; var fSize: int64; var Crc: Cardinal);
function NetIsWork: boolean;
implementation

function NetIsWork: boolean;
var
  f: TextFile;
  i: Integer;
  s: String;
begin
  for i := 0 to 9 do
  begin
    s := '/sys/class/net/eth'+IntToStr(i)+'/carrier';
    Result := FileExists(s);
    if Result then
      break;
  end;
  if Result then
  begin
    Assign(f, s);
    Reset(f);
    ReadLn(f, i);
    CloseFile(f);
    Result := i = 1;
  end;
end;

function GetLocalIP(t: TIdSocketHandles): string;
begin
  Result := '';
  if t.Count >= 1 then
    Result := t.Items[0].PeerIP;
end;

procedure CastMyMessage(d: string);
var
  i, k: Integer;
  b: TIdBytes;
begin
  if IdIPMCastServer1 <> nil then
  begin
    k := Length(d);
    SetLength(b, k);
    for i := 1 to k do
      b[i - 1] := Ord(d[i]);
    IdIPMCastServer1.Send(b);
    b := nil;
  end;
end;

procedure Str2List(l: TStrings; d, f: string);
var k: Integer;
begin
  l.Clear;
  k := PosEx(f, d);
  while k > 0 do
  begin
    l.Add(copy(d, 1, k - 1));
    d := copy(d, k + 1, length(d) - k);
    k := PosEx(f, d);
  end;
  l.Add(d);
end;
{$DEFINE MemMode}
{$IFDEF MemMode}
{$ELSE}
{$ENDIF}

procedure ClientSendFile(fsName, SvrIp: string);
var
{$IFDEF MemMode}
  fsStream: TMemoryStream;
{$ELSE}
  fsStream: TFileStream;
{$ENDIF}
  iFileLen, ASize: Integer;
  c: Cardinal;
  s: string;
  t: TIdTCPClient;
begin
  t := TIdTCPClient.Create(AOwner);
  with t do
  begin
    Host := SvrIp;
    Port := 20001;
    IPVersion := Id_IPv4;
    //BoundIP:=LocalIP;
    //BoundPort:=20001;
    ConnectTimeout := 5000;
    ReadTimeout := 5000;
    try
      Connect;
    except
      exit;
    end;
  end;
  if t.Connected then
  begin
{$IFDEF MemMode}
    fsStream := TMemoryStream.Create;
    fsStream.LoadFromFile(fsName);
    c := CRC32Of(fsStream);
{$ELSE}
    c := CRC32Of(fsName);
    fsStream := TFileStream.Create(fsName, fmOpenRead);
{$ENDIF}
    iFileLen := fsStream.Size;
    //fsName:=LowerCase(fsName);
    //s := ExtractFileDrive(fsName);
    s := Copy(fsName, Length(WorkDir), Length(fsName));
    t.IOHandler.WriteLn(Format('%s|%d*%.8X', [s, iFileLen, c]));
    //s + '|' + IntToStr(iFileLen)
    //s := t.IOHandler.ReadLn;
    //ASize := StrToInt(s);

    fsStream.Seek(0, soFromBeginning);
    repeat
      ASize := fsStream.Size - fsStream.Position;
      if ASize > t.IOHandler.SendBufferSize then
        ASize := t.IoHandler.SendBufferSize;
      //IoHandler.WriteBufferOpen;
      t.IoHandler.Write(fsStream, Asize);
      //IoHandler.WriteBufferClose;
    until fsStream.Position = iFileLen;
    fsStream.Free;
    t.Disconnect;
  end;
  t.Free;
end;

function GetFileFromTcpIp(fsName: string; fsSize: Int64; CTxt: TIdContext; Crc: Cardinal = 0): boolean;
var
{$IFDEF MemMode}
  fsStream: TMemoryStream;
{$ELSE}
  fsStream: TFileStream;
{$ENDIF}
  ASize: Int64;
begin
  Result := True;
{$IFDEF MemMode}
  fsStream := TMemoryStream.Create;
{$ELSE}
  fsStream := TFileStream.Create(fsName, fmCreate);
{$ENDIF}
  try
    //CTxt.Connection.IOHandler.Writeln(IntToStr(sFile.Size));
    repeat
      ASize := fsSize - fsStream.Size;
      if ASize > CTxt.Connection.IOHandler.RecvBufferSize then
        ASize := CTxt.Connection.IOHandler.RecvBufferSize;
      CTxt.Connection.IOHandler.ReadStream(fsStream, ASize);
    until fsStream.Size = fsSize;
  finally
    if Crc <> 0 then
      Result := Crc = CRC32Of(fsStream);
{$IFDEF MemMode}
    fsStream.SaveToFile(fsName);
{$ENDIF}
    fsStream.Free;
  end;
end;

procedure GetFileSizeCrc(s: string; var fSize: int64; var Crc: Cardinal);
var
  i: Integer;
begin
  i := PosEx('*', s);
  fSize := StrToInt(Copy(s, 1, i - 1));
  Crc := StrToInt('$' + Copy(s, i + 1, Length(s) - i + 1));
end;
end.


