﻿
#include "LibLsp/lsp/general/exit.h"
#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "LibLsp/lsp/general/initialize.h"
#include "LibLsp/lsp/ProtocolJsonHandler.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include <network/uri.hpp>
#include "LibLsp/JsonRpc/Condition.h"
#include "LibLsp/lsp/textDocument/did_change.h"
#include "LibLsp/lsp/textDocument/did_save.h"	
#include "LibLsp/JsonRpc/Endpoint.h"
#include "LibLsp/JsonRpc/TcpServer.h"
#include "LibLsp/lsp/textDocument/document_symbol.h"
#include "LibLsp/lsp/workspace/execute_command.h"
#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include "LibLsp/lsp/textDocument/references.h"
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <iostream>

#include "LibLsp/lsp/ClientPreferences.h"
#include "CompilationUnit.h"
#include "LibLsp/lsp/workspace/didChangeWorkspaceFolders.h"
#include "LibLsp/lsp/textDocument/hover.h"
#include "LibLsp/lsp/textDocument/completion.h"
#include "WorkSpaceManager.h"

#include "LibLsp/lsp/utils.h"
#include "LibLsp/lsp/working_files.h"
#include "LibLsp/lsp/SimpleTimer.h"
#include "lpg2/IcuUtil.h"
#include "message/MessageHandler.h"
#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "lpg2/Monitor.h"
#include "LibLsp/lsp/ParentProcessWatcher.h"
#include "LibLsp/lsp/textDocument/resolveCompletionItem.h"
#include "LibLsp/lsp/textDocument/formatting.h"
#include "LibLsp/lsp/textDocument/documentColor.h"
#include "LibLsp/lsp/general/shutdown.h"
#include "LibLsp/lsp/workspace/did_change_watched_files.h"
#include "LibLsp/lsp/general/initialized.h"
#include "LibLsp/JsonRpc/cancellation.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "LibLsp/lsp/textDocument/rename.h"
#include "LibLsp/lsp/lsAny.h"
#include "LibLsp/lsp/workspace/did_change_configuration.h"
#include "LibLsp/lsp/client/registerCapability.h"
#include "LibLsp/lsp/workspace/symbol.h"
#include "LibLsp/lsp/textDocument/code_action.h"
#include "lpg2/Monitor.h"
#include "lpg2/stringex.h"
using namespace boost::asio::ip;
using namespace std;
using namespace lsp;


#include <thread>
#include <atomic>
#include <functional>
#include <boost/asio.hpp>

MAKE_REFLECT_STRUCT(GenerationOptions,  template_search_directory,
	include_search_directory, package,  visitor, language, trace, quiet, verbose,  additionalParameters);

REFLECT_MAP_TO_STRUCT(GenerationOptions,  template_search_directory, language,
	include_search_directory,  package,  visitor, trace, quiet, verbose,  additionalParameters);

class DummyLog :public lsp::Log
{
public:

	void log(Level level, std::wstring&& msg)
	{
		std::wcout << msg << std::endl;
	};
	void log(Level level, const std::wstring& msg)
	{
		std::wcout << msg << std::endl;
	};
	void log(Level level, std::string&& msg)
	{
		std::cout << msg << std::endl;
	};
	void log(Level level, const std::string& msg)
	{
		std::cout << msg << std::endl;
	};
};

std::string _address = "127.0.0.1";
bool ShouldIgnoreFileForIndexing(const std::string& path) {
	return StartsWith(path, "git:");
}

class Server
{
public:
	

	WorkSpaceManager   work_space_mgr;

	RemoteEndPoint& _sp;
	

	boost::optional<Rsp_Error> need_initialize_error;
	
	bool enable_watch_parent_process = false;
	std::unique_ptr<ParentProcessWatcher> parent_process_watcher;
	std::shared_ptr<ClientPreferences>clientPreferences;


	struct ExitMsgMonitor : Monitor
	{
		std::atomic_bool is_running_ = true;
		bool isCancelled()  override
		{
			return  !is_running_.load(std::memory_order_relaxed);
		}
		void Cancel()
		{
			is_running_.store(false, std::memory_order_relaxed);
		}
	};
	struct RequestMonitor : Monitor
	{
		RequestMonitor(ExitMsgMonitor& _exit, const CancelMonitor& _cancel) :exit(_exit), cancel(_cancel)
		{

		}

		bool isCancelled() override
		{
			if (exit.isCancelled()) return  true;
			if (cancel && cancel())
			{
				return  true;
			}
			return  false;
		}
		ExitMsgMonitor& exit;
		const CancelMonitor& cancel;
	};
	ExitMsgMonitor exit_monitor;
	

	std::shared_ptr<CompilationUnit> GetUnit(const lsTextDocumentIdentifier& uri, Monitor* monitor, bool keep_consist = true)
	{
		const AbsolutePath& path = uri.uri.GetAbsolutePath();
		auto unit = work_space_mgr.find(path);
		if (unit)
		{
			
			if (unit->NeedToCompile() && keep_consist)
			{
				unit =work_space_mgr.Build(unit->working_file, monitor);
			}
		}
		else
		{
			unit = work_space_mgr.CreateUnit(path, monitor);
		}
		return  unit;
	}
	
	void on_exit()
	{
		exit_monitor.Cancel();
		server.stop();
		esc_event.notify(std::make_unique<bool>(true));
	}
	std::map<std::string, Registration> registeredCapabilities;
	
	void collectRegisterCapability(const std::string& method) {
		if (registeredCapabilities.find(method) == registeredCapabilities.end()) {
			auto reg = Registration::Create(method);
			registeredCapabilities[method] = reg;
		}
	}

	Server(const std::string& _port ,bool _enable_watch_parent_process) :work_space_mgr(server.point, _log),
	_sp(server.point), enable_watch_parent_process(_enable_watch_parent_process), server(_address, _port, protocol_json_handler, endpoint, _log)
	{

		need_initialize_error = Rsp_Error();
		need_initialize_error->error.code = lsErrorCodes::ServerNotInitialized;
		need_initialize_error->error.message = "Server is not initialized";
		
		
		_sp.registerHandler([=](const td_initialize::request& req)
			{
				need_initialize_error.reset();
				clientPreferences = std::make_shared<ClientPreferences>(req.params.capabilities) ;
				td_initialize::response rsp;
				lsServerCapabilities capabilities;
				auto SETTINGS_KEY = "settings";
			
				if(req.params.initializationOptions)
				{
					do
					{
						map<std::string, lsp::Any> initializationOptions;
						try
						{
							lsp::Any init_object = req.params.initializationOptions.value();
						
							init_object.Get(initializationOptions);
						}
						catch (...)
						{
							break;
						}
						map<std::string, lsp::Any> settings;
						try
						{
							
							initializationOptions[SETTINGS_KEY].Get(settings);
						}
						catch (...)
						{
							break;
						}
						GenerationOptions generation_options;
					
						try
						{
						
 							settings["options"].GetFromMap(generation_options);
						}
						catch (...)
						{
							break;
						}
						
						
						work_space_mgr.UpdateSetting(generation_options);
					}
					while (false);	
				}
		
				
				std::pair<boost::optional<lsTextDocumentSyncKind>,
				          boost::optional<lsTextDocumentSyncOptions> > textDocumentSync;
				lsTextDocumentSyncOptions options;
				options.openClose = true;
				options.change = lsTextDocumentSyncKind::Incremental;
				options.willSave = false;
				options.willSaveWaitUntil = false;
				textDocumentSync.second = options;
				capabilities.textDocumentSync = textDocumentSync;
				
			    if(!clientPreferences->isHoverDynamicRegistered())
			    {
					capabilities.hoverProvider = true;
			    }
				if (!clientPreferences->isCompletionDynamicRegistered())
				{
					lsCompletionOptions completion;
					completion.resolveProvider = true;
					capabilities.completionProvider = completion;
				}
				std::pair< boost::optional<bool>, boost::optional<WorkDoneProgressOptions> > option;
				option.first = true;
			
				if (!clientPreferences->isDefinitionDynamicRegistered())
				{
					capabilities.definitionProvider = option;
				}
				if (!clientPreferences->isFoldgingRangeDynamicRegistered())
				{
					capabilities.foldingRangeProvider = std::pair< boost::optional<bool>, boost::optional<FoldingRangeOptions> >();
					capabilities.foldingRangeProvider->first = true;
				}
				if (!clientPreferences->isReferencesDynamicRegistered())
				{
					capabilities.referencesProvider = option;
				}
				if (!clientPreferences->isDocumentSymbolDynamicRegistered())
				{
					capabilities.documentSymbolProvider = option;
				}
				if (!clientPreferences->isFormattingDynamicRegistrationSupported())
				{
					capabilities.documentFormattingProvider = option;
				}
				if (!clientPreferences->isRenameDynamicRegistrationSupported())
				{
					std::pair< boost::optional<bool>, boost::optional<RenameOptions> > rename_opt;
					rename_opt.first = true;
					capabilities.renameProvider = rename_opt;
				}
				
				{

					SemanticTokensWithRegistrationOptions semantic_tokens_opt;
					auto  semanticTokenTypes = [] {
						std::vector< std::string>  _type;
						for (unsigned i = 0; i <= static_cast<unsigned>(SemanticTokenType::lastKind);
							++i)
							_type.push_back(to_string(static_cast<SemanticTokenType>(i)));
						return _type;
					};

					semantic_tokens_opt.legend.tokenTypes = semanticTokenTypes();

					std::pair< boost::optional<bool>, boost::optional<lsp::Any> > rang;
					rang.first = false;
					semantic_tokens_opt.range = rang;

					std::pair< boost::optional<bool>,
						boost::optional<SemanticTokensServerFull> > full;
					full.first = true;

					semantic_tokens_opt.full = full;
					capabilities.semanticTokensProvider = std::move(semantic_tokens_opt);
				}
			
				rsp.result.capabilities.swap(capabilities);
				WorkspaceServerCapabilities workspace_server_capabilities;
				//capabilities.workspace
			    if(req.params.processId.has_value() && _enable_watch_parent_process)
			    {
					parent_process_watcher = std::make_unique<ParentProcessWatcher>(_log,req.params.processId.value(),
						[&](){
							on_exit();
					});
			    }
				return  std::move(rsp);
			});
		_sp.registerHandler([&](Notify_InitializedNotification::notify& notify)
		{

				if (!clientPreferences)return;
				
				if (clientPreferences->isCompletionDynamicRegistered())
				{
					collectRegisterCapability(td_completion::request::kMethodInfo);
				}
				if (clientPreferences->isWorkspaceSymbolDynamicRegistered())
				{
					collectRegisterCapability(wp_symbol::request::kMethodInfo);
				}
				if (clientPreferences->isDocumentSymbolDynamicRegistered())
				{
					collectRegisterCapability(td_symbol::request::kMethodInfo);
				}
				/*if (clientPreferences->isCodeActionDynamicRegistered())
				{
					collectRegisterCapability(td_codeAction::request::kMethodInfo);
				}*/
				if (clientPreferences->isDefinitionDynamicRegistered())
				{
					collectRegisterCapability(td_definition::request::kMethodInfo);
				}
				if (clientPreferences->isHoverDynamicRegistered())
				{
					collectRegisterCapability(td_hover::request::kMethodInfo);
				}
				if (clientPreferences->isReferencesDynamicRegistered())
				{
					collectRegisterCapability(td_references::request::kMethodInfo);
				}
				if (clientPreferences->isFoldgingRangeDynamicRegistered())
				{
					collectRegisterCapability(td_foldingRange::request::kMethodInfo);
				}
				if (clientPreferences->isWorkspaceFoldersSupported())
				{
					collectRegisterCapability(Notify_WorkspaceDidChangeWorkspaceFolders::notify::kMethodInfo);
				}
				if(clientPreferences->isWorkspaceDidChangeConfigurationSupported())
				{
					collectRegisterCapability(Notify_WorkspaceDidChangeConfiguration::notify::kMethodInfo);
				}
			
				Req_ClientRegisterCapability::request request;
				for(auto& it : registeredCapabilities)
				{
					request.params.registrations.push_back(it.second);
				}
				_sp.send(request);
				
		});
		_sp.registerHandler(
			[&](const td_symbol::request& req, const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_symbol::response > {
				if(need_initialize_error)
				{
					return need_initialize_error.value();
				}
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit =GetUnit(req.params.textDocument,&_requestMonitor);
				td_symbol::response rsp;
				if (unit){
					rsp.result = unit->document_symbols;
				}
				return std::move(rsp);
			});
		_sp.registerHandler(
			[&](const td_definition::request& req, const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_definition::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument,&_requestMonitor);
				td_definition::response rsp;
				rsp.result.first = std::vector<lsLocation>();
				if (unit){
					process_definition(unit, req.params.position, rsp.result.first.value(), &_requestMonitor);
				}
				return std::move(rsp);
			});
		
		_sp.registerHandler(
			[&](const td_hover::request& req, const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_hover::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				td_hover::response rsp;
			/*	if(req_back.params.uri == req.params.uri && req_back.params.position == req.params.position && req_back.params.textDocument.uri == req.params.textDocument.uri)
				{
					return std::move(rsp);
				}
				else
				{
					req_back = req;
				}*/
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument,&_requestMonitor);
				
				if (unit)
				{
					process_hover(unit, req.params.position, rsp.result, &_requestMonitor);
					if(_requestMonitor.isCancelled())
					{
						rsp.result.contents.second.reset();
						rsp.result.contents.first = TextDocumentHover::Left();
						return std::move(rsp);
					}
					if(!rsp.result.contents.first.has_value() && !rsp.result.contents.second.has_value())
					{
						rsp.result.contents.first = TextDocumentHover::Left();
					}
				}
				else
				{
					rsp.result.contents.first = TextDocumentHover::Left();
				}

				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_completion::request& req
			, const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_completion::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor,true);
				td_completion::response rsp;
				if (unit){
					CompletionHandler(unit, rsp.result, req.params,&_requestMonitor);
				}
				
				
				return std::move(rsp);
				
			});
		_sp.registerHandler([&](const completionItem_resolve::request& req)
			{
				completionItem_resolve::response rsp;
				rsp.result = req.params;
				return std::move(rsp);
			});
		
		_sp.registerHandler([&](const td_foldingRange::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_foldingRange::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument,&_requestMonitor);
				td_foldingRange::response rsp;
				if (unit){
					FoldingRangeHandler(unit, rsp.result, req.params);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_formatting::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_formatting::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				td_formatting::response rsp;
				auto unit = GetUnit(req.params.textDocument,&_requestMonitor);
				if (unit){
					DocumentFormatHandler(unit, rsp.result, req.params.options);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_documentColor::request& req 	,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_documentColor::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				td_documentColor::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit){
					DocumentColorHandler(unit, rsp.result);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_semanticTokens_full::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_semanticTokens_full::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				td_semanticTokens_full::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					SemanticTokens tokens;
					SemanticTokensHandler(unit, tokens);
					rsp.result = std::move(tokens);
				}
				
				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_references::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_references::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				td_references::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit){
					ReferencesHandler(unit, req.params.position, rsp.result,&_requestMonitor);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const td_rename::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< td_rename::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				td_rename::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					std::vector< lsWorkspaceEdit::Either >  edits;
					RenameHandler(unit, req.params, edits, &_requestMonitor);
					rsp.result.documentChanges = std::move(edits);
				
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_inlineNonTerminal::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_inlineNonTerminal::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_inlineNonTerminal::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					InlineNonTerminalHandler give_me_a_name(unit, req.params, rsp.result, &_requestMonitor);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_makeEmpty::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_makeEmpty::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_makeEmpty::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
				
					MakeEmptyNonTerminalHandler give_me_a_name(unit, req.params, rsp.result, &_requestMonitor);
				
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_makeNonEmpty::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_makeNonEmpty::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_makeNonEmpty::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					
					MakeNonEmptyNonTerminalHandler give_me_a_name(unit, req.params, rsp.result, &_requestMonitor);
		
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_MakeLeftRecursive::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_MakeLeftRecursive::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_MakeLeftRecursive::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					
					MakeLeftRecursiveHandler give_me_a_name(unit, req.params, rsp.result, &_requestMonitor);
		

				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_call_graph::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_call_graph::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_call_graph::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					CallGraphHandler give_me_a_name(unit,  rsp.result, &_requestMonitor);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_rrd_allRules::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_rrd_allRules::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_rrd_allRules::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params, &_requestMonitor);
				if (unit) {
					AanlyseForAllRule(unit, rsp.result, &_requestMonitor, AnalysePurpose::For_RRD);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_rrd_singleRule::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_rrd_singleRule::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_rrd_singleRule::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					AanlyseSingleRule(unit, req.params, rsp.result, &_requestMonitor, AnalysePurpose::For_RRD);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_firstSet_allRules::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_firstSet_allRules::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_firstSet_allRules::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params, &_requestMonitor);
				if (unit) {
					AanlyseForAllRule(unit, rsp.result, &_requestMonitor, AnalysePurpose::For_FirstSet);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_firstSet_singleRule::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_firstSet_singleRule::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_firstSet_singleRule::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					AanlyseSingleRule(unit, req.params, rsp.result, &_requestMonitor, AnalysePurpose::For_FirstSet);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_FollowSet_allRules::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_FollowSet_allRules::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_FollowSet_allRules::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params, &_requestMonitor);
				if (unit) {
					AanlyseForAllRule(unit, rsp.result, &_requestMonitor, AnalysePurpose::For_FollowSet);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](const lpg_FollowSet_singleRule::request& req,
			const CancelMonitor& monitor)
			->lsp::ResponseOrError< lpg_FollowSet_singleRule::response > {
				if (need_initialize_error)
				{
					return need_initialize_error.value();
				}
				lpg_FollowSet_singleRule::response rsp;
				RequestMonitor _requestMonitor(exit_monitor, monitor);
				auto unit = GetUnit(req.params.textDocument, &_requestMonitor);
				if (unit) {
					AanlyseSingleRule(unit, req.params, rsp.result, &_requestMonitor, AnalysePurpose::For_FollowSet);
				}
				return std::move(rsp);
			});
		_sp.registerHandler([&](Notify_TextDocumentDidOpen::notify& notify)
			{
				if (need_initialize_error){
					return ;
				}
				auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				work_space_mgr.OnOpen(params.textDocument,&exit_monitor);
			});
	
		_sp.registerHandler([&]( Notify_TextDocumentDidChange::notify& notify)
		{
			if (need_initialize_error) {
				return;
			}
			const auto& params = notify.params;
			AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
			if (ShouldIgnoreFileForIndexing(path))
				return;
			work_space_mgr.OnChange(params,&exit_monitor);
			
		});
		_sp.registerHandler([&](Notify_TextDocumentDidClose::notify& notify)
		{
				if (need_initialize_error) {
					return;
				}
				//Timer time;
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				work_space_mgr.OnClose(params.textDocument);

		});
		
		_sp.registerHandler([&](Notify_TextDocumentDidSave::notify& notify)
			{
				if (need_initialize_error) {
					return;
				}
				//Timer time;
				const auto& params = notify.params;
				AbsolutePath path = params.textDocument.uri.GetAbsolutePath();
				if (ShouldIgnoreFileForIndexing(path))
					return;
				work_space_mgr.OnSave(params.textDocument);
				// 通知消失了
			});
		
		_sp.registerHandler([&](Notify_WorkspaceDidChangeWatchedFiles::notify& notify)
		{
		    
		});
		_sp.registerHandler([&](Notify_WorkspaceDidChangeConfiguration::notify& notify)
			{
				do
				{
					map<std::string, lsp::Any> settings;
					try
					{
						notify.params.settings.Get(settings);
					}
					catch (...)
					{
						break;
					}
					GenerationOptions generation_options;

					try
					{
						settings["options"].GetFromMap(generation_options);
					}
					catch (...)
					{
						break;
					}
					work_space_mgr.UpdateSetting(generation_options);
				} while (false);
			
			});
		_sp.registerHandler([&](Notify_WorkspaceDidChangeWorkspaceFolders::notify& notify)
		{
				if (need_initialize_error) {
					return;
				}
				work_space_mgr.OnDidChangeWorkspaceFolders(notify.params);
		});
		
		_sp.registerHandler([&](const td_shutdown::request& notify) {
			td_shutdown::response rsp;
			return rsp;
		});
		_sp.registerHandler([&](Notify_Exit::notify& notify)
		{
				on_exit();
		});

		std::thread([&]()
			{
				server.run();
			}).detach();
	}
	~Server()
	{
		server.stop();
	}
	std::shared_ptr < lsp::ProtocolJsonHandler >  protocol_json_handler = std::make_shared < lsp::ProtocolJsonHandler >();
	DummyLog  _log;

	std::shared_ptr < GenericEndpoint >  endpoint = std::make_shared<GenericEndpoint>(_log);
	lsp::TcpServer server;
	Condition<bool> esc_event;

};

const char VERSION[] = "LPG-language-server 0.2.3 (" __DATE__ ")";

const char* _PORT_STR = "port";
int main(int argc, char* argv[])
{
	
	using namespace  boost::program_options;
	options_description desc(" LPG-language-server allowed options");
	desc.add_options()
		(_PORT_STR, value<int>(), "tcp port")
		("watchParentProcess", "enable watch parent process")
		("help,h", "produce help message")
		("version,v", VERSION);


	variables_map vm;
	try {
		store(parse_command_line(argc, argv, desc), vm);
	}
	catch (std::exception& e) {
		std::cout << "Undefined input.Reason:" << e.what() << std::endl;
		return 0;
	}
	notify(vm);


	if (vm.count("help") || vm.count("h"))
	{
		cout << desc << endl;
		return 1;
	}
	if (vm.count("version") ||  vm.count("v"))
	{
		cout << VERSION << endl;
		return 1;
	}
	bool  enable_watch_parent_process = false;
	if (vm.count("watchParentProcess"))
	{
		enable_watch_parent_process = true;
	}
	int  _port = 9333;
	if (vm.count(_PORT_STR))
	{
		cout << "Tcp port was set to " << vm[_PORT_STR].as<int>() << "." << endl;
		_port = vm[_PORT_STR].as<int>();
	}
	else
	{
		cout << "Please set TCP port ."  << endl;
		return -1;
	}
	Server server(stringex().format("%d",_port), enable_watch_parent_process);
	auto ret =server.esc_event.wait();
	if(ret)
	{
		return 0;
	}
	return -1;
}


