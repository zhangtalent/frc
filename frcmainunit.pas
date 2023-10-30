unit FrcMainUnit;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, FileUtil, LResources, Forms, Controls, Graphics, Dialogs,
  StdCtrls, IdGlobal, IdIPMCastClient, IdTCPServer, IdIPMCastServer,
  IdSocketHandle, IdContext;

type

  { TFrCMainForm }

  TFrCMainForm = class(TForm)
    Button1: TButton;
    Button3: TButton;
    IdIPMCastServer1: TIdIPMCastServer;
    zkzEdit: TEdit;
    gzwjjEdit: TEdit;
    IdIPMCastClient1: TIdIPMCastClient;
    IdTCPServer1: TIdTCPServer;
    Label1: TLabel;
    Label2: TLabel;
    procedure Button1Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: boolean);
    procedure FormCreate(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure IdIPMCastClient1IPMCastRead(Sender: TObject;
      const AData: TIdBytes; ABinding: TIdSocketHandle);
    procedure IdTCPServer1Execute(AContext: TIdContext);
  private
    { private declarations }
     Msg: TStringList;
     isexit:boolean;
    procedure varinit;
    procedure tcpipinit;
 public
    { public declarations }
    procedure CheckDebug(p:TStrings);
  end;

var
  FrCMainForm: TFrCMainForm;

implementation

uses FunLib, ConstUnit, pubFunUnit;

{ TFrCMainForm }

procedure TFrCMainForm.varinit;
begin
  WorkDir:= GetUserDir+'Desktop/';//'/home/noi/桌面/';
  gzwjjEdit.Text := WorkDir;
  zkzEdit.Clear;
  zkzEdit.Text:='';//test   FJ-999
  WorkMsg := '*' + zkzEdit.Text + '*' + WorkDir;
  CastMyMessage('1');
  LocalIP:=GetLocalIP(IdIPMCastClient1.Bindings);
end;

procedure TFrCMainForm.tcpipinit;
var
  t: TIdSocketHandle;
begin
  //uses idGlobal  -dUseCThreads
  //explicitally adding a Binding object prevents TIdTCPServer
  //from creating its own default IPv4 and IPv6 Binding objects
  //on the same listening IP/Port pair...
  t := IdTCPServer1.Bindings.Add;
  t.IPVersion := Id_IPv4; //optional: forces the Binding to work in Id_IPV4 mode.
  t.IP := '';
  t.Port := 20001; //customization
  IdTCPServer1.Active := True;

  t:=IdIPMCastClient1.Bindings.Add;
  t.IPVersion := Id_IPv4;
  t.IP:='';//GetLocalIP1;//'192.168.0.21';
  t.Port:=20000;
  IdIPMCastClient1.Active:=true;

  with IdIPMCastServer1 do
  begin
    Port:=20000;
    Active:=true;
  end;
end;

procedure TFrCMainForm.CheckDebug(p: TStrings);
var
  i: Integer;
  s1, s2: string;
  function RandExt(c: Integer): string;
  begin
    case c of
      0: Result := '.c';
      1: Result := '.pas';
    else Result := '.cpp';
    end;
  end;
  procedure CreateRandFile(f: string);
  var
    t: TextFile;
    j, k: Integer;
  begin
    AssignFile(t, f);
    Rewrite(t);
    Writeln(t, f);
    for j := 1 to 100 + Random(100) do
    begin
      for k := 1 to 10 + Random(40) do write(t, chr(48 + Random(122 - 48 + 1)));
      Writeln(t);
    end;
    Writeln(t, s2);
    Writeln(t, s1);
    CloseFile(t);
  end;
begin
  s1:=p.Strings[0];
  if Copy(s1,1,6) = '-Debug' then
  if zkzEdit.Text='' then
  begin
    //ShowMessage(ParamStr(1));
    Randomize;
    i := length(LocalIP);
    while LocalIP[i] <> '.' do
      dec(i);
    s1 := copy(LocalIP, i + 1, 3);
    while length(s1) < 3 do s1 := '0' + s1;
    s1 := 'Test-' + s1;
    zkzEdit.Text := s1;
    s1 := gzwjjEdit.Text + s1;
    for i := 1 to p.Count-1 do
    begin
      s2 := s1;
      if p.Strings[0]='-Debug1' then
      s2:=s2 + '/' + p.Strings[i];
      if ForceDirectories(s2) then
      begin
        CreateRandFile(s2 + '/' + p.Strings[i] + RandExt(Random(3)));
      end;
    end;
    WorkMsg := '*' + zkzEdit.Text + '*' + gzwjjEdit.Text;
    CastMyMessage('5' + ServerIP + WorkMsg);
  end;
end;

procedure TFrCMainForm.FormCreate(Sender: TObject);
var i:integer;
begin
  AOwner:=Self;
  pubFunUnit.IdIPMCastServer1 := IdIPMCastServer1;
  //pubFunUnit.IdIPMCastClient1 := IdIPMCastClient1;
  Msg := TStringList.Create;
  //LocalIP := GetLocalIP;
  Randomize;
  if NetIsWork then
  begin
    tcpipinit;
    varinit;
    for i:=1 to Paramcount do
      if Copy(ParamStr(i),1,2)='-w' then
      begin
        //gzwjjEdit.Text:= Copy(ParamStr(i),3,Length(ParamStr(i)));
        WorkDir:= GetUserDir+Copy(ParamStr(i),3,Length(ParamStr(i)))+'/';//'/home/noi/桌面/';
        gzwjjEdit.Text := WorkDir;
        WorkMsg := '*' + zkzEdit.Text + '*' + WorkDir;
        CastMyMessage('1');
        break;
      end;
  end
  else
  begin
    ShowMessage('当前没有网络连接，请在网络正常情况下启动程序！');
    //Button3Click(nil);
    isexit:=True;
    Close;
    //Application.Terminate;
  end;
end;

procedure TFrCMainForm.FormShow(Sender: TObject);
begin
  if isexit then Close;
end;

procedure TFrCMainForm.IdIPMCastClient1IPMCastRead(Sender: TObject;
  const AData: TIdBytes; ABinding: TIdSocketHandle);
var
  t: TStringList;
  i: Integer;
begin
  t := TStringList.Create;
  i:=IdIPMCastClientIPMCastRead(AData, ABinding.PeerIP, Msg, t);
  if i = 2 then
  begin
    //IdTCPServer1.Active:=False;
    for i := 1 to t.Count - 1 do
      if FileExists(t.Strings[0]+t.Strings[i]) then
      begin
        Msg.Add('Send File:' + t.Strings[i]);
        ClientSendFile(t.Strings[0]+t.Strings[i], ServerIP);
      end;
    CastMyMessage('7' + ServerIP);
    //IdTCPServer1.Active:=True;
  end
  else if i = 3 then
    CheckDebug(t)
  else
  begin
    Caption := ServerIP+'*'+LocalIP;
    //LocalIP:=GetLocalIP(IdIPMCastClient1.Bindings);
  end;
  t.Free;
end;

procedure TFrCMainForm.IdTCPServer1Execute(AContext: TIdContext);
begin
  ServerReceiveFile(AContext, Msg);
end;

procedure TFrCMainForm.Button1Click(Sender: TObject);
begin
  WorkDir:= gzwjjEdit.Text;
  WorkMsg := '*' + zkzEdit.Text + '*' + WorkDir;
  if {(ServerIP = '') or }(zkzEdit.Text = '') or (gzwjjEdit.Text = '') then
    //ShowMessage('Code or Work Can not be EMPTY!')
    ShowMessage('准考证号、工作文件夹不能为空！')
  else
  if NetIsWork then
  begin
    //while ((LocalIP='')or(ServerIP='')) do
    if ((LocalIP='')or(ServerIP='')) then
    begin
      CastMyMessage('1');
      LocalIP:=GetLocalIP(IdIPMCastClient1.Bindings);
      Caption := ServerIP+'*'+LocalIP;
    end;
    CastMyMessage('5' + ServerIP + WorkMsg);
    FrCMainForm.WindowState:=wsMinimized;
  end
  else
    ShowMessage('当前没有网络连接，请在网络正常情况下再按［确定］！');
end;


procedure TFrCMainForm.Button3Click(Sender: TObject);
begin
  if Sender<>nil then
    isexit:=MessageDlg('退出系统将不会自动传输文件，确定？',mtConfirmation,[mbYes,mbCancel],0)=mrYes
  else isexit:=True;
  Close;
end;


procedure TFrCMainForm.FormCloseQuery(Sender: TObject; var CanClose: boolean);
begin
  CanClose:=isexit;
  if CanClose then   //Once exit,will not automatically submitted infomation,confirm?
  begin
    Msg.Add('work message '+WorkMsg);
    Msg.Add('Server ip '+ServerIP);
    Msg.Add('Local ip '+LocalIP);
    Msg.Add(IntToStr(WorkCode));
    Msg.SaveToFile('FrC.log');
  end;
end;


initialization
  {$I frcmainunit.lrs}

end.

