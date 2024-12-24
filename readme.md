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

### 설정 적용
```bash
# vsftpd 서비스 재시작
sudo systemctl restart vsftpd
```