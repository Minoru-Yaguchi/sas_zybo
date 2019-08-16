import os
import boto3
from boto3.dynamodb.conditions import Key, Attr
from datetime import datetime as dt
from flask import abort

class DynamoDB:
    def __init__(self):
        '''
        初期化
        '''
        # DynamoDBの参照先取得
        self.dynamodb = boto3.resource('dynamodb')
        self.table = self.dynamodb.Table('auth_log_data_table')


    def getlist_item(self):
        '''
        DynamoDBから要素一覧を取得する関数
        '''
        # scan関数で一覧取得
        res = self.table.scan(FilterExpression=Attr('DeviceID').eq('id001'))

        # Datetimeで昇順に並び替え
        items = res['Items']
        items = sorted(items, key=lambda x:x['Datetime'], reverse=True)

        return items

    def search_item(self, tdate):
        '''
        DynamoDBから特定要素を取得する関数(Datetimeで検索)
        '''
        # string形式からdate形式に変換
        date = dt.strptime(tdate, '%Y%m%d%H%M%S')

        # query関数で一致するデータを検索
        res = self.table.query(KeyConditionExpression=Key('DeviceID').eq('id001') & Key('Datetime').eq(date.strftime('%Y-%m-%d %H:%M:%S')))
        items = res['Items']

        if(len(items) == 0):        # 見つからない場合には404エラーを返す
            abort(404, { 'data': 'Page' })
        else:                       # 見つかった場合には最初のデータを返す
            item = items[0]

        return item

    def adddictforList(self, results):
        '''
        一覧表示用に文字列の変換処理を行う
        '''
        for r in results:
            date = dt.strptime(r['Datetime'], '%Y-%m-%d %H:%M:%S')
            r['path'] = date.strftime('%Y%m%d%H%M%S')
            r['DatetimeJPN'] = date.strftime('%Y年%m月%d日 %H時%M分%S秒')
        return results

    def filecheckexists(self, path, fname):
        '''
        ファイル存在確認
        '''
        return os.path.exists('/static/' + path + fname)

    def checkforResult(self, result):
        '''
        結果表示用にデータを生成する
        '''
        # 結果表示用画像データが存在するかどうか確認、ない場合には404エラーを返す
        if(self.filecheckexists('zybo/', result['PictureName'])):
            abort(404, { 'data': 'Zybo Image' })
        if(self.filecheckexists('face/', result['Person'] + '.jpg')):
            abort(404, { 'data': 'Face Image' })
        
        return result
