★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★

TDEforPG 対応 psqlODBC の評価について、以下のように実施してください。

1. PostgreSQLサーバーを準備してください。
   
2. 本 PostgreSQLサーバーの template1データベースに対してTDEforPG機能を
   有効化してください。

3. テストを行う環境(Windows)において、以下の空フォルダを作成してください。

   D:/SWF/Jenkins/workspace/psqlodbc_log_dont_delete/

4. 一旦 regress.bat -Platform both を実行してください。

   テストソースビルド後に、接続入力要求がありますので、評価PostgreSQL
   サーバーの情報を指定してください。

5. 4.を実施後に、それぞれの 32ビット・64ビットの psqlodbc_test_dsn
   というDSNができるため、以下のように編集してください。
   
   5.1) PostgreSQL Unicode ODBC セットアップ画面(psqlodbc_test_dsn設定
        画面)の「全体設定」ボタンを押下してください。
        
   5.2) 表示される画面上で、ロギング用フォルダの右のテキストボックスに
        以下を入力して、保存してください。
        
        D:\SWF\Jenkins\workspace\psqlodbc_log_dont_delete\

6. regress.bat -Platform bothを再度実施して評価を行ってください。


★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★