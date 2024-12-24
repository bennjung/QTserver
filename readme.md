# VSFTPD 서버 구성 가이드

## 설치 방법

### Ubuntu 에서 vsftpd 설치
```bash
# vsftpd 패키지 설치
sudo apt-get update
sudo apt-get install vsftpd

# vsftpd 설정 파일 편집
sudo nano /etc/vsftpd.conf
```

### 주요 설정 변경 항목

1. 익명 사용자 로그인 비활성화
```
# 기본값: anonymous_enable=YES
anonymous_enable=NO
```

2. 로컬 사용자 로그인 활성화
```
# 기본값: local_enable=NO
local_enable=YES
```

3. 파일 업로드/쓰기 권한 설정
```
# 기본값: write_enable=NO
write_enable=YES
```

4. 보안 강화를 위한 설정
```
# 로컬 사용자의 홈 디렉토리 외부 접근 제한
chroot_local_user=YES
```

###FTP 사용자 및 디렉토리 설정
1. FTP 전용 사용자 생성
```
# FTP 사용자 생성 (사용자 이름은 원하는 대로 설정 가능)
sudo useradd -m [username]
sudo [passwd] [username]
```

2. FTP 루트 디렉토리 설정
```
# FTP 루트 디렉토리 생성
sudo mkdir -p /srv/ftp

# 디렉토리 소유권 및 권한 설정
sudo chown -R username:username /srv/ftp
sudo chmod 755 /srv/ftp
```

3. vsftpd.conf에 루트 디렉토리 설정 추가 
```
# FTP 루트 디렉토리 설정
local_root=/srv/ftp
```

### 설정 적용
```bash
# vsftpd 서비스 재시작
sudo systemctl restart vsftpd
```

### 주의사항
- FTP 사용자 이름과 비밀번호는 보안을 위해 적절히 설정해주세요 
- 채팅 클라이언트 실행 시 FTP 접속 정보를 입력하라는 메시지가 표시됩니다
- 입력한 FTP 접속 정보는 config.ini 파일에 저장됩니다