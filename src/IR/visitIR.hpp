/**
 * 用于活跃变量分析
 */
#include "custom_ir.hpp"
#include <unordered_map>
#include <unordered_set>
class LiveVarAnalysis {
  public:
  std::unordered_map<std::string, std::unordered_set<std::string>> live_out;
  std::unordered_map<std::string, std::unordered_set<std::string>> live_in;
  
  void scanf(IR::IRProgram *program) {
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto func = program->funcs.rbegin(); func != program->funcs.rend(); func++) {
        for (auto block = (*func)->blocks.rbegin(); block != (*func)->blocks.rend(); block++) {
          auto old_live_in = live_in[(*block)->name];
          auto var_kill = std::unordered_set<std::string>();
          auto var_gen = std::unordered_set<std::string>();
          hard_code_block(*block, var_gen, var_kill);
          live_out[(*block)->name].clear();
          // for S in successors(B): LIVEOUT[B] = LIVEOUT[B].union(LIVEIN[S])
          if ((*block)->next_1) {
            live_out[(*block)->name].insert(live_in[(*block)->next_1->name].begin(), live_in[(*block)->next_1->name].end());
            if ((*block)->next_2) {
              live_out[(*block)->name].insert(live_in[(*block)->next_2->name].begin(), live_in[(*block)->next_2->name].end());
            }
          }
          live_out[(*block)->name].erase(var_kill.begin(), var_kill.end());
          live_out[(*block)->name].insert(var_gen.begin(), var_gen.end());
          live_in[(*block)->name] = live_out[(*block)->name];
          if (old_live_in != live_in[(*block)->name]) {
            changed = true;
          }
        }
      }
    }
  }

  // 遍历得到Gen和Kill
  void hard_code_block(std::shared_ptr<IR::IRBasicBlock> block, std::unordered_set<std::string> &var_gen, std::unordered_set<std::string> &var_kill) { 
    for (auto inst = block->insts.rbegin(); inst != block->insts.rend(); inst++) { 
      
    }
  }
};