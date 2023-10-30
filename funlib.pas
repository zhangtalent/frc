unit FunLib;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, StrUtils,
  IdContext, IdGlobal;

function IdIPMCastClientIPMCastRead(AData: TIdBytes; const IP: string; vMSG: TStrings; fnames: TStrings): Integer;
procedure ServerReceiveFile(CTxt: TIdContext; vMSG: TStrings = nil);

implementation

uses ConstUnit, pubFunUnit;

function IdIPMCastClientIPMCastRead(AData: TIdBytes; const IP: string; vMSG: TStrings; fnames: TStrings): Integer;
var
  k: Integer;
  MsgTxt, s: string;
begin
  Result := -1;
  MsgTxt:='';
  for k:=0 to Length(AData)-1 do
  MsgTxt:=MsgTxt+chr(AData[k]);

  vMSG.Add(MsgTxt);
  if MsgTxt <> '' then
  begin
    //Result := StrToInt(copy(bf.DataString, 1, 1));
    case MsgTxt[1] of
      '2':
        begin
          s := copy(MsgTxt, 2, 16);
          if s = LocalIP then
          begin
            CastMyMessage('3' + ServerIP);
          end; //}
        end;
      '4':
        begin
          ServerIP := IP;
          //CastMyIP(IdIPMCastServer, '5' + ServerIP + WorkMsg);
        end;
      '8':
        begin //
          ServerIP := IP;
          CastMyMessage('5' + ServerIP + WorkMsg);
          //LocalIP:=GetLocalIP(IdIPMCastClient1.Bindings);
          if WorkCode=0 then
          begin
            WorkCode:=Random(65535);
            CastMyMessage('9' + IntToStr(WorkCode));
          end;
        end;
      'B':
        begin //
          //{
          k := PosEx('*', MsgTxt);
          s := copy(MsgTxt, 2, k - 2); //
          if s = LocalIP then //
          begin
            msgTxt := copy(MsgTxt, k + 1, length(MsgTxt) - k);
            vMSG.Add(IP + ':' + msgTxt);
            Result := 2; //
            str2List(fnames, MsgTxt, '*');
          end; //}
        end;
      'D':
        begin
          s := copy(MsgTxt, 2, 16);
          if LocalIP <>'' then
          begin
            if s = LocalIP then
            begin
              ServerIP := IP;
              CastMyMessage('5' + ServerIP + WorkMsg);
            end;
          end
          else
          begin
            //{
            k:=Length(MsgTxt);
            s:='';
            while MsgTxt[k]<>'.' do
            begin
              s:=MsgTxt[k]+s;
              dec(k);
            end;
            if s='255' then
            begin
              ServerIP := IP;
              WorkCode:=Random(65535);
              CastMyMessage('9' + IntToStr(WorkCode));
            end;//}
          end;
        end;
      'F':
        begin //
          s := copy(MsgTxt, 3, Length(MsgTxt));
          k := PosEx('*', s);
          if StrToInt(Copy(s,1,k-1)) = WorkCode then
          begin
            LocalIP := Copy(s,k+1,16);
            CastMyMessage('5' + ServerIP + WorkMsg);
          end; //}
        end;
      'H':  //
        if LocalIP='' then
        begin
          s := copy(MsgTxt, 3, Length(MsgTxt));
          k := PosEx('*', s);
          if StrToInt(Copy(s,1,k-1)) = WorkCode then
          begin
            WorkCode:=Random(65535);
            CastMyMessage('9' + IntToStr(WorkCode));
          end; //}
        end;
      'Z':
        begin
          MsgTxt := copy(MsgTxt, 2, length(MsgTxt) - 1);
          str2List(fnames, MsgTxt, '*');
          Result := 3;
        end;
    end;
  end;
end;

procedure ServerReceiveFile(CTxt: TIdContext; vMSG: TStrings = nil);
var
  i:Integer;
  FileSize: Int64;
  crc:Cardinal;
  str, FileName: string;
begin
  str := CTxt.Connection.IOHandler.ReadLn;
  if vMSG <> nil then vMSG.Add(str);
  i := PosEx('|', str);
  FileName := copy(str, 1, i - 1);
  str := copy(str, i + 1, Length(str) - i + 1);
  GetFileSizeCrc(str,FileSize,crc);
  //FileName := WorkDir + '/' + ExtractFileName(LowerCase(FileName));
  FileName := WorkDir + ExtractFileName(FileName);
  if not GetFileFromTcpIp(FileName,FileSize,CTxt,crc) then
    if vMSG <> nil then vMSG.Add(FileName+'Crc error!');
end;

end.

