HW6 - grep from directory


Amaç : Verilen bir klasör içindeki tüm okunur dosyalarda belli bir kelimeyi arayıp bulunan koorinatlari log dosyasinda tutmak.


#Neler Yapıldı - Nasil Çalışır ? 


##USAGE
- make
- ./exec dirname word

##Thread Yapisi
- Her bir okunur dosya (txt, word ...) için thread oluşturuldu ve paralel olarak dosyalarda aramalar yapildi.
- Her thread bulduğu konumlari bir linked list içinde depolayıp daha sonra bunlari log dosyasina basti.
- Loglarin karışmamasi için NAMED_SEMAPHORE kullanildi. 
- Threadler bulduklari toplam sayilari ise Semafor ile olusturulan mutex ile Message-queue ye yazarlar.


##Fork yapisi
- Her directroy için fork yapılarak aynı thread işlemleri tekrarlandı ve toplam klasör içindeki tekrar sayisi
(threadlerin message-queuelere yazdigi degerler ) shared-memory ile üst klasörlere aktarıldı.
- Shared-memory karışıklığı olmamasi için NAMED_SEMAPHORE kullanıldı.

##Sinyal Yapisi
- Karişiklik olmamasi için sadece SIGINT ( ^C ) sinyali tutuldu. 
- Sinyal gelme durumunda global degisken set edilir ve program normal bir sekilde bitiş işlemine devam eder.
- Sinyal gelene kadar yapilan işlemler loglara TOPLAM SÜRELERİYLE beraber yazilir.
- Log dosyasina ve ekrana sinyal yakalandiğina dair bilgi basilir.


##Normal Çalışma İşlemleri ve Log
- Sinyal olmamasi durumunda normal olarak devam eden arama sonucu ortaya çıkan tüm sonuçlar log dosyalarina
ilk kapan yazar mantığıyla yazilir. 
- Programin toplam çalışma süresi milisaniye(ms) cinsinden hem log hemde ekrana yazilir.
- Loglarin sonunda toplam tekrar sayisida eklenmiştir.


## Valgrind
- Olasi tüm leakler yok edildi.
- Dinamic bellekler yok edilip heap'e geri kazandırıldılar.

